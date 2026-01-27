# LibreDCC Sculpt Engine Architecture

## Design Document v0.1
**Date:** January 27, 2026  
**Status:** Proposal  
**Author:** Architecture Discussion

---

## 1. Executive Summary

This document proposes a **Spatial Micro-ECS Architecture** for LibreDCC's sculpting engine. The core insight is that sculpting operations are spatially coherent—a brush only affects a local region of vertices. By treating only the brush-affected region as "alive" in an ECS sense, we can achieve ZBrush-level performance (50-100M+ polygons) on commodity hardware.

### Key Principles

1. **Cold/Hot Separation** - Most mesh data is dormant; only brush region is active
2. **Spatial Indexing** - Octree/BVH for fast region queries
3. **Micro-ECS** - ECS-style batch processing for active vertices only
4. **Lazy Evaluation** - Only compute what changed
5. **Cache Efficiency** - Active data fits in CPU cache

---

## 2. The Problem

### Traditional Approach

```cpp
// Naive sculpting - processes ALL vertices
class Mesh {
    std::vector<Vertex> vertices;  // 50 million vertices
};

void sculpt(Mesh& mesh, vec3 brushPos, float radius) {
    for (auto& v : mesh.vertices) {           // 50M iterations
        float dist = distance(v.pos, brushPos);
        if (dist < radius) {
            v.pos += offset * falloff(dist);  // Only ~10K actually affected
        }
    }
}
```

**Problems:**
- Iterating 50M vertices to modify 10K
- Poor cache utilization (Vertex struct has pos, normal, UV, color, tangent...)
- Can't parallelize efficiently
- Memory bandwidth bottleneck

### Performance Reality

| Vertices | Naive Approach | Target |
|----------|---------------|--------|
| 1M | ~16ms | <1ms |
| 10M | ~160ms | <2ms |
| 50M | ~800ms | <5ms |
| 100M | Unusable | <10ms |

**60fps requires <16ms frame time. Naive approach fails at 1M vertices.**

---

## 3. Proposed Architecture

### Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                     SCULPT ENGINE                                │
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                  COLD STORAGE                            │    │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐        │    │
│  │  │ Positions   │ │ Normals     │ │ UVs         │        │    │
│  │  │ (50M vec3)  │ │ (50M vec3)  │ │ (50M vec2)  │        │    │
│  │  └─────────────┘ └─────────────┘ └─────────────┘        │    │
│  │  Structure-of-Arrays, memory mapped, compressed          │    │
│  └─────────────────────────────────────────────────────────┘    │
│                              │                                   │
│                       activate/deactivate                        │
│                              │                                   │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                  SPATIAL INDEX                           │    │
│  │                                                          │    │
│  │     ┌───┬───┐      Octree or BVH                        │    │
│  │     │   │   │      • O(log n) ray intersection          │    │
│  │     ├───┼───┤      • O(log n) radius queries            │    │
│  │     │ ● │   │      • Stores vertex indices per node     │    │
│  │     └───┴───┘      • Rebuilt on topology change         │    │
│  │                                                          │    │
│  └─────────────────────────────────────────────────────────┘    │
│                              │                                   │
│                     query affected region                        │
│                              │                                   │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │               HOT WORKING SET (Micro-ECS)                │    │
│  │                                                          │    │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────┐        │    │
│  │  │ Indices     │ │ Positions   │ │ Normals     │        │    │
│  │  │ (~50K)      │ │ (~50K)      │ │ (~50K)      │        │    │
│  │  └─────────────┘ └─────────────┘ └─────────────┘        │    │
│  │  ┌─────────────┐ ┌─────────────┐                        │    │
│  │  │ Weights     │ │ Dirty Flags │  Cache-friendly        │    │
│  │  │ (~50K)      │ │ (~50K)      │  SIMD-ready            │    │
│  │  └─────────────┘ └─────────────┘  Parallelizable        │    │
│  │                                                          │    │
│  └─────────────────────────────────────────────────────────┘    │
│                              │                                   │
│                        write back                                │
│                              ▼                                   │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                  GPU SYNC                                │    │
│  │  • Partial buffer updates (only changed vertices)        │    │
│  │  • Double buffered for async                             │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 4. Data Structures

### 4.1 Cold Storage (Full Mesh)

```cpp
// Structure-of-Arrays for cache efficiency
struct ColdMesh {
    // Geometry (always needed)
    std::vector<glm::vec3> positions;      // 50M * 12 bytes = 600MB
    std::vector<glm::vec3> normals;        // 50M * 12 bytes = 600MB
    
    // Attributes (loaded on demand)
    std::vector<glm::vec2> uvs;            // Optional
    std::vector<glm::vec4> colors;         // Optional
    std::vector<glm::vec4> tangents;       // Optional
    
    // Topology
    std::vector<uint32_t> indices;         // Triangles
    std::vector<VertexAdjacency> adjacency; // For normal calculation
    
    // Subdivision hierarchy (future)
    std::vector<SubdivLevel> subdivLevels;
};
```

**Why Structure-of-Arrays (SoA)?**

```
Array-of-Structures (AoS) - BAD for sculpting:
┌─────────────────────────────────────────────────────┐
│ Vertex0: [pos][normal][uv][color][tangent]          │
│ Vertex1: [pos][normal][uv][color][tangent]          │
│ ...                                                  │
└─────────────────────────────────────────────────────┘
Cache loads: pos, normal, uv, color, tangent (80 bytes)
Cache uses:  pos (12 bytes) = 15% utilization

Structure-of-Arrays (SoA) - GOOD for sculpting:
┌─────────────────────────────────────────────────────┐
│ Positions: [pos0][pos1][pos2][pos3][pos4]...        │
│ Normals:   [n0][n1][n2][n3][n4]...                  │
│ UVs:       [uv0][uv1][uv2][uv3][uv4]...             │
└─────────────────────────────────────────────────────┘
Cache loads: pos0, pos1, pos2, pos3 (48 bytes)
Cache uses:  all of it = 100% utilization
```

### 4.2 Spatial Index (Octree)

```cpp
struct OctreeNode {
    BoundingBox bounds;
    
    // Leaf data
    std::vector<uint32_t> vertexIndices;  // Which vertices live here
    
    // Tree structure
    std::array<OctreeNode*, 8> children;  // nullptr if leaf
    OctreeNode* parent;
    
    // State
    bool isActive;        // Is this node in hot working set?
    uint32_t lastUsed;    // Frame number for LRU eviction
    
    // Methods
    bool isLeaf() const { return children[0] == nullptr; }
    bool intersects(const Sphere& brush) const;
    bool contains(const vec3& point) const;
};

struct Octree {
    OctreeNode* root;
    uint32_t maxDepth;
    uint32_t maxVerticesPerLeaf;  // Split threshold
    
    // Queries
    void queryRadius(vec3 center, float radius, std::vector<OctreeNode*>& out);
    void queryRay(Ray ray, std::vector<OctreeNode*>& out);
    void queryFrustum(Frustum frustum, std::vector<OctreeNode*>& out);
    
    // Maintenance
    void build(const ColdMesh& mesh);
    void refit(const std::vector<uint32_t>& changedVertices);
};
```

### 4.3 Hot Working Set (Micro-ECS)

```cpp
struct HotWorkingSet {
    // Indices back to cold storage
    std::vector<uint32_t> indices;
    
    // Active data (SoA layout)
    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec3> originalPositions;  // For stroke undo
    
    // Brush data
    std::vector<float> weights;        // Brush falloff weights
    std::vector<uint8_t> dirtyFlags;   // What needs recalculation
    
    // Acceleration
    std::vector<OctreeNode*> activeNodes;  // Which octree nodes are hot
    
    // Stats
    size_t count() const { return indices.size(); }
    size_t memoryUsage() const;
    
    // Operations
    void clear();
    void activate(OctreeNode* node, const ColdMesh& mesh);
    void deactivate(OctreeNode* node, ColdMesh& mesh);
    void writeBack(ColdMesh& mesh);
};
```

### 4.4 Dirty Flags

```cpp
enum DirtyFlag : uint8_t {
    DIRTY_NONE     = 0,
    DIRTY_POSITION = 1 << 0,
    DIRTY_NORMAL   = 1 << 1,
    DIRTY_UV       = 1 << 2,
    DIRTY_COLOR    = 1 << 3,
    DIRTY_GPU      = 1 << 4,  // Needs GPU buffer update
    DIRTY_OCTREE   = 1 << 5,  // Moved enough to refit octree
};
```

---

## 5. Algorithms

### 5.1 Brush Stroke Lifecycle

```cpp
class SculptBrush {
public:
    void beginStroke(vec3 hitPoint, vec3 hitNormal, float radius) {
        // 1. Store initial state
        strokeStart_ = hitPoint;
        strokeRadius_ = radius;
        
        // 2. Query spatial index for affected region
        octree_.queryRadius(hitPoint, radius * 1.5f, affectedNodes_);
        
        // 3. Activate nodes into hot working set
        for (OctreeNode* node : affectedNodes_) {
            if (!node->isActive) {
                workingSet_.activate(node, coldMesh_);
            }
        }
        
        // 4. Store original positions for undo
        workingSet_.originalPositions = workingSet_.positions;
        
        // 5. Calculate brush weights
        calculateWeights(hitPoint, radius);
    }
    
    void updateStroke(vec3 currentPoint, vec3 delta) {
        // 1. Expand working set if brush moved to new region
        expandWorkingSetIfNeeded(currentPoint);
        
        // 2. Recalculate weights for new brush position
        calculateWeights(currentPoint, strokeRadius_);
        
        // 3. Apply brush operation (SIMD optimized)
        applyBrush(delta);
        
        // 4. Mark vertices dirty
        markDirty(DIRTY_POSITION | DIRTY_NORMAL | DIRTY_GPU);
    }
    
    void endStroke() {
        // 1. Recalculate normals for affected region
        recalculateNormals();
        
        // 2. Write back to cold storage
        workingSet_.writeBack(coldMesh_);
        
        // 3. Update GPU buffers (partial update)
        gpuSync_.updatePartial(workingSet_.indices, workingSet_.positions, workingSet_.normals);
        
        // 4. Push undo state
        undoStack_.push(createUndoState());
        
        // 5. Optionally keep working set warm for next stroke
        // Or deactivate: workingSet_.clear();
    }
};
```

### 5.2 Weight Calculation (SIMD Optimized)

```cpp
void calculateWeights(vec3 brushCenter, float brushRadius) {
    const size_t count = workingSet_.count();
    const float invRadius = 1.0f / brushRadius;
    
    // SIMD: Process 4 vertices at a time
    size_t i = 0;
    for (; i + 4 <= count; i += 4) {
        // Load 4 positions
        __m128 px = _mm_set_ps(
            workingSet_.positions[i+0].x,
            workingSet_.positions[i+1].x,
            workingSet_.positions[i+2].x,
            workingSet_.positions[i+3].x
        );
        // ... similar for py, pz
        
        // Calculate distances
        __m128 dx = _mm_sub_ps(px, _mm_set1_ps(brushCenter.x));
        __m128 dy = _mm_sub_ps(py, _mm_set1_ps(brushCenter.y));
        __m128 dz = _mm_sub_ps(pz, _mm_set1_ps(brushCenter.z));
        
        __m128 distSq = _mm_add_ps(
            _mm_add_ps(_mm_mul_ps(dx, dx), _mm_mul_ps(dy, dy)),
            _mm_mul_ps(dz, dz)
        );
        __m128 dist = _mm_sqrt_ps(distSq);
        
        // Calculate falloff (smoothstep)
        __m128 t = _mm_mul_ps(dist, _mm_set1_ps(invRadius));
        t = _mm_min_ps(_mm_set1_ps(1.0f), t);
        __m128 weight = smoothstep_simd(t);
        
        // Store weights
        _mm_store_ps(&workingSet_.weights[i], weight);
    }
    
    // Handle remainder
    for (; i < count; i++) {
        float dist = glm::distance(workingSet_.positions[i], brushCenter);
        float t = std::min(1.0f, dist * invRadius);
        workingSet_.weights[i] = smoothstep(1.0f, 0.0f, t);
    }
}
```

### 5.3 Apply Brush (Parallelized)

```cpp
void applyBrush(vec3 offset) {
    const size_t count = workingSet_.count();
    
    // Parallel processing with thread pool
    parallel_for(0, count, [&](size_t i) {
        float w = workingSet_.weights[i];
        if (w > 0.001f) {
            workingSet_.positions[i] += offset * w;
            workingSet_.dirtyFlags[i] |= DIRTY_POSITION;
        }
    });
}
```

### 5.4 Normal Recalculation (Only Affected Region)

```cpp
void recalculateNormals() {
    // Only recalculate normals for dirty vertices
    // Use adjacency information from cold mesh
    
    parallel_for(0, workingSet_.count(), [&](size_t i) {
        if (!(workingSet_.dirtyFlags[i] & DIRTY_POSITION)) return;
        
        uint32_t vertIdx = workingSet_.indices[i];
        const VertexAdjacency& adj = coldMesh_.adjacency[vertIdx];
        
        vec3 normal(0);
        for (uint32_t faceIdx : adj.faces) {
            // Get face vertices (may be in working set or cold storage)
            vec3 v0 = getPosition(coldMesh_.indices[faceIdx * 3 + 0]);
            vec3 v1 = getPosition(coldMesh_.indices[faceIdx * 3 + 1]);
            vec3 v2 = getPosition(coldMesh_.indices[faceIdx * 3 + 2]);
            
            normal += glm::cross(v1 - v0, v2 - v0);
        }
        
        workingSet_.normals[i] = glm::normalize(normal);
        workingSet_.dirtyFlags[i] |= DIRTY_NORMAL;
    });
}

vec3 getPosition(uint32_t vertIdx) {
    // Check if vertex is in working set
    auto it = workingSetLookup_.find(vertIdx);
    if (it != workingSetLookup_.end()) {
        return workingSet_.positions[it->second];
    }
    // Fall back to cold storage
    return coldMesh_.positions[vertIdx];
}
```

### 5.5 Partial GPU Update

```cpp
void updateGPUPartial() {
    // Collect ranges of dirty vertices
    std::vector<Range> dirtyRanges = collectDirtyRanges();
    
    // Update only dirty portions of GPU buffer
    for (const Range& range : dirtyRanges) {
        size_t offset = range.start * sizeof(GPUVertex);
        size_t size = range.count * sizeof(GPUVertex);
        
        // Build GPU data for this range
        std::vector<GPUVertex> gpuData;
        for (size_t i = range.start; i < range.start + range.count; i++) {
            uint32_t coldIdx = workingSet_.indices[i];
            gpuData.push_back({
                workingSet_.positions[i],
                workingSet_.normals[i],
                coldMesh_.uvs[coldIdx]
            });
        }
        
        // Partial buffer update (Vulkan)
        void* mapped;
        vkMapMemory(device_, stagingMemory_, offset, size, 0, &mapped);
        memcpy(mapped, gpuData.data(), size);
        vkUnmapMemory(device_, stagingMemory_);
        
        // Copy to GPU buffer
        VkBufferCopy copyRegion = { offset, offset, size };
        vkCmdCopyBuffer(cmd, stagingBuffer_, vertexBuffer_, 1, &copyRegion);
    }
}
```

---

## 6. Performance Analysis

### 6.1 Expected Performance

| Metric | Traditional | Spatial Micro-ECS |
|--------|-------------|-------------------|
| Vertices processed | 50,000,000 | ~50,000 |
| Cache misses | Millions | Hundreds |
| Memory bandwidth | ~4 GB/s | ~50 MB/s |
| Parallelization | Limited | Full |
| Frame time (brush) | ~800ms | ~2ms |

### 6.2 Memory Budget

```
Cold Storage (50M vertices):
  Positions:  600 MB
  Normals:    600 MB
  UVs:        400 MB
  Adjacency:  800 MB
  Total:     ~2.4 GB

Octree (50M vertices):
  Nodes:      ~50 MB
  Indices:    ~200 MB
  Total:     ~250 MB

Hot Working Set (50K vertices max):
  Indices:    200 KB
  Positions:  600 KB
  Normals:    600 KB
  Weights:    200 KB
  Total:     ~1.6 MB  ← Fits in L3 cache!

GPU Buffers:
  Vertex buffer: ~2.4 GB
  Index buffer:  ~600 MB
```

### 6.3 Why It's Fast

1. **Hot set fits in cache**
   - L3 cache: 8-32 MB on modern CPUs
   - Working set: ~1.6 MB
   - Zero cache misses during brush operation

2. **SIMD efficiency**
   - Processing 4-8 vertices per instruction
   - 4x-8x throughput improvement

3. **Parallelization**
   - 8-16 cores on modern CPUs
   - Linear scaling for embarrassingly parallel operations

4. **Minimal memory bandwidth**
   - Only touching ~50K vertices, not 50M
   - 1000x reduction in memory traffic

---

## 7. Future Enhancements

### 7.1 GPU Compute Brushes

```cpp
// Compute shader for brush application
// Process on GPU instead of CPU

layout(local_size_x = 256) in;

layout(std430, binding = 0) buffer Positions { vec3 positions[]; };
layout(std430, binding = 1) buffer Weights { float weights[]; };

uniform vec3 brushOffset;

void main() {
    uint i = gl_GlobalInvocationID.x;
    if (i >= positionCount) return;
    
    positions[i] += brushOffset * weights[i];
}
```

### 7.2 Predictive Activation

```cpp
void updateStroke(vec3 currentPoint, vec3 velocity) {
    // Predict where brush will be next frame
    vec3 predictedPoint = currentPoint + velocity * predictTime_;
    
    // Pre-warm nodes in predicted region
    std::vector<OctreeNode*> predictedNodes;
    octree_.queryRadius(predictedPoint, strokeRadius_, predictedNodes);
    
    for (OctreeNode* node : predictedNodes) {
        if (!node->isActive) {
            // Activate asynchronously on background thread
            asyncActivate(node);
        }
    }
}
```

### 7.3 Level-of-Detail Sculpting

```cpp
// Coarse stroke on low subdivision level
// Detail preserved at higher levels

void sculptWithLOD(vec3 brushPos, float radius, int lodLevel) {
    // 1. Apply brush at requested LOD
    applyBrushAtLOD(brushPos, radius, lodLevel);
    
    // 2. Propagate changes to higher LODs
    for (int lod = lodLevel + 1; lod <= maxLOD_; lod++) {
        propagateToHigherLOD(lod);
    }
    
    // 3. Propagate constraints to lower LODs
    for (int lod = lodLevel - 1; lod >= 0; lod--) {
        propagateToLowerLOD(lod);
    }
}
```

### 7.4 Out-of-Core Streaming

```cpp
// For meshes larger than RAM

struct StreamingMesh {
    MemoryMappedFile positionsFile;
    MemoryMappedFile normalsFile;
    
    // Only frequently accessed pages stay in RAM
    // OS handles paging automatically
    
    vec3 getPosition(uint32_t idx) {
        return positionsFile.read<vec3>(idx * sizeof(vec3));
    }
};
```

---

## 8. Implementation Phases

### Phase 1: Basic Infrastructure (2-3 weeks)
- [ ] Implement SoA mesh storage
- [ ] Basic octree construction
- [ ] Simple radius queries
- [ ] Manual hot/cold activation

### Phase 2: Core Sculpting (3-4 weeks)
- [ ] Brush stroke lifecycle
- [ ] Weight calculation
- [ ] Basic brush types (grab, smooth, inflate)
- [ ] Normal recalculation

### Phase 3: Optimization (2-3 weeks)
- [ ] SIMD weight calculation
- [ ] Parallel brush application
- [ ] Partial GPU updates
- [ ] Working set pooling

### Phase 4: Polish (2-3 weeks)
- [ ] Undo/redo system
- [ ] Stroke smoothing
- [ ] Pressure sensitivity
- [ ] More brush types

### Phase 5: Advanced (Future)
- [ ] GPU compute brushes
- [ ] Predictive activation
- [ ] LOD sculpting
- [ ] Dynamic topology

---

## 9. Open Questions

1. **Octree vs BVH?**
   - Octree: Simpler, good for uniform distribution
   - BVH: Better for non-uniform, used by Blender

2. **When to deactivate nodes?**
   - Immediately after stroke?
   - LRU eviction with max working set size?
   - Keep warm for N frames?

3. **GPU compute vs CPU SIMD?**
   - CPU: Simpler, lower latency
   - GPU: Higher throughput, but transfer overhead

4. **Memory mapping vs explicit loading?**
   - mmap: Simpler, OS handles paging
   - Explicit: More control, predictable performance

---

## 10. References

- Blender PBVH: `source/blender/blenkernel/intern/pbvh.c`
- Data-Oriented Design: Mike Acton's CppCon talks
- SIMD Programming: Intel Intrinsics Guide
- Vulkan Buffer Updates: Vulkan spec Chapter 12

---

## 11. Glossary

| Term | Definition |
|------|------------|
| **SoA** | Structure of Arrays - data layout for cache efficiency |
| **AoS** | Array of Structures - traditional OOP layout |
| **Hot** | Actively processed, in cache |
| **Cold** | Dormant, in main memory or disk |
| **SIMD** | Single Instruction Multiple Data - parallel CPU operations |
| **BVH** | Bounding Volume Hierarchy - spatial acceleration structure |
| **PBVH** | Parallel BVH - Blender's sculpting acceleration structure |
| **LOD** | Level of Detail - multiple resolution representations |

---

*Document Version: 0.1*  
*Last Updated: January 27, 2026*
