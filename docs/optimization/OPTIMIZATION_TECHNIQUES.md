# Optimization Techniques

This document outlines the key optimization approaches used in the Astral engine to achieve high performance in cellular physics simulation.

## Core Optimization Principles

1. **Measure Before Optimizing**: Always profile to identify bottlenecks
2. **Focus on Hot Paths**: Optimize code that runs most frequently
3. **Data-Oriented Design**: Organize data for cache efficiency
4. **Parallelism**: Utilize multi-threading and SIMD
5. **Minimize Memory Allocations**: Avoid allocations in performance-critical paths
6. **Early Culling**: Skip work that won't affect the final output

## Memory Optimizations

### Cache-Friendly Data Layouts

```cpp
// Bad: Object-oriented approach with poor cache locality
class Cell {
    MaterialID material;
    float temperature;
    float pressure;
    glm::vec2 velocity;
    // Other properties...
};
std::vector<Cell> cells;

// Good: Struct of arrays for better cache locality
struct CellData {
    std::vector<MaterialID> materials;
    std::vector<float> temperatures;
    std::vector<float> pressures;
    std::vector<glm::vec2> velocities;
    // Other properties...
};
```

### Custom Memory Allocators

```cpp
template<typename T, size_t BlockSize = 4096>
class PoolAllocator {
private:
    struct Block {
        char data[BlockSize];
        Block* next;
    };
    
    struct FreeNode {
        FreeNode* next;
    };
    
    Block* currentBlock;
    FreeNode* freeList;
    std::size_t objectSize;
    
public:
    PoolAllocator() : currentBlock(nullptr), freeList(nullptr) {
        objectSize = sizeof(T) < sizeof(FreeNode) ? sizeof(FreeNode) : sizeof(T);
    }
    
    ~PoolAllocator() {
        while (currentBlock) {
            Block* temp = currentBlock;
            currentBlock = currentBlock->next;
            delete temp;
        }
    }
    
    T* allocate() {
        if (freeList) {
            T* result = reinterpret_cast<T*>(freeList);
            freeList = freeList->next;
            return result;
        }
        
        if (!currentBlock || (BlockSize - currentBlockOffset) < objectSize) {
            Block* newBlock = new Block;
            newBlock->next = currentBlock;
            currentBlock = newBlock;
            currentBlockOffset = 0;
        }
        
        T* result = reinterpret_cast<T*>(currentBlock->data + currentBlockOffset);
        currentBlockOffset += objectSize;
        return result;
    }
    
    void deallocate(T* object) {
        if (!object) return;
        
        FreeNode* node = reinterpret_cast<FreeNode*>(object);
        node->next = freeList;
        freeList = node;
    }
};
```

### Object Pooling

```cpp
template<typename T>
class ObjectPool {
private:
    std::vector<std::unique_ptr<T>> objects;
    std::vector<T*> freeObjects;
    
public:
    ObjectPool(size_t initialSize = 100) {
        objects.reserve(initialSize);
        freeObjects.reserve(initialSize);
        
        for (size_t i = 0; i < initialSize; ++i) {
            objects.push_back(std::make_unique<T>());
            freeObjects.push_back(objects.back().get());
        }
    }
    
    T* acquire() {
        if (freeObjects.empty()) {
            objects.push_back(std::make_unique<T>());
            return objects.back().get();
        }
        
        T* object = freeObjects.back();
        freeObjects.pop_back();
        return object;
    }
    
    void release(T* object) {
        if (!object) return;
        freeObjects.push_back(object);
    }
};
```

## Physics Simulation Optimizations

### Active Cell Tracking

Instead of updating every cell in the grid, maintain a list of active cells to update:

```cpp
class ChunkManager {
    // ...
    std::unordered_set<ChunkCoord, ChunkCoordHash> activeChunks;
    
    void updateActiveState(const ChunkCoord& coord, bool active) {
        if (active) {
            activeChunks.insert(coord);
        } else {
            activeChunks.erase(coord);
        }
    }
    
    void updatePhysics(float deltaTime) {
        // Only update active chunks
        for (const auto& coord : activeChunks) {
            Chunk* chunk = getChunk(coord);
            if (chunk) {
                chunk->update(deltaTime);
            }
        }
    }
    // ...
};
```

### Checkerboard Pattern Updates

To avoid race conditions in parallel updates, use a checkerboard pattern:

```cpp
void PhysicsSystem::updateParallel(float deltaTime) {
    // First update "black" cells on checkerboard
    parallelFor(0, activeChunks.size(), [&](size_t i) {
        ChunkCoord coord = activeChunks[i];
        if ((coord.x + coord.y) % 2 == 0) {
            Chunk* chunk = getChunk(coord);
            if (chunk) {
                chunk->update(deltaTime);
            }
        }
    });
    
    // Then update "white" cells
    parallelFor(0, activeChunks.size(), [&](size_t i) {
        ChunkCoord coord = activeChunks[i];
        if ((coord.x + coord.y) % 2 == 1) {
            Chunk* chunk = getChunk(coord);
            if (chunk) {
                chunk->update(deltaTime);
            }
        }
    });
}
```

### Material-Specific Optimization

Different update frequencies for different materials:

```cpp
void CellularPhysics::updateMaterial(int x, int y, float deltaTime) {
    Cell& cell = grid[y][x];
    const MaterialProperties& props = materials.get(cell.material);
    
    // Skip updates for non-movable materials
    if (!props.movable) return;
    
    // Different update frequencies based on material
    if (props.type == MaterialType::SOLID) {
        // Solids rarely need updates
        if (frameCount % 10 != 0) return;
    } else if (props.type == MaterialType::POWDER) {
        // Powders update every frame
    } else if (props.type == MaterialType::LIQUID) {
        // Viscous liquids update less frequently
        if (props.viscosity > 0.5f && frameCount % 2 != 0) return;
    } else if (props.type == MaterialType::GAS) {
        // Gases might update more frequently
        // Could update multiple times per frame for faster gases
    }
    
    // Perform the actual update...
}
```

### Dirty Rectangle Tracking

Track only the regions that need updating:

```cpp
class PhysicsSystem {
    // ...
    std::vector<Rect> dirtyRects;
    
    void markDirty(int x, int y, int width = 1, int height = 1) {
        dirtyRects.push_back({x, y, width, height});
    }
    
    void update(float deltaTime) {
        // Merge overlapping dirty rects
        dirtyRects = mergeRects(dirtyRects);
        
        // Only update cells in dirty regions
        for (const auto& rect : dirtyRects) {
            for (int y = rect.y; y < rect.y + rect.height; ++y) {
                for (int x = rect.x; x < rect.x + rect.width; ++x) {
                    updateCell(x, y, deltaTime);
                }
            }
        }
        
        dirtyRects.clear();
    }
    // ...
};
```

## Rendering Optimizations

### Instanced Rendering

Render many cells with a single draw call:

```cpp
void GridRenderer::renderInstanced(const ChunkManager* chunkManager) {
    // Setup instance data
    std::vector<glm::vec4> instanceData;
    
    // For each visible cell
    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            Cell& cell = chunkManager->getCell(x, y);
            
            // Skip empty cells
            if (cell.material == MaterialID::AIR) continue;
            
            // Add position and material data
            instanceData.emplace_back(x, y, (float)cell.material, 0.0f);
        }
    }
    
    // Upload instance data
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(glm::vec4), 
                instanceData.data(), GL_STREAM_DRAW);
    
    // Draw all cells in one call
    glBindVertexArray(VAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, instanceData.size());
}
```

### Texture Atlasing

Combine material textures into a single atlas:

```cpp
class TextureAtlas {
    // ...
    GLuint atlasTexture;
    std::unordered_map<MaterialID, glm::vec4> uvCoordinates;
    
    glm::vec4 getMaterialUV(MaterialID material) const {
        auto it = uvCoordinates.find(material);
        if (it != uvCoordinates.end()) {
            return it->second;
        }
        return glm::vec4(0, 0, 1, 1); // Default UV
    }
    // ...
};
```

### Frustum Culling

Only render chunks visible to the camera:

```cpp
bool Chunk::isVisibleFrom(const Camera& camera) const {
    // Calculate chunk bounds in world space
    BoundingBox bounds = {
        glm::vec3(coord.x * CHUNK_SIZE, coord.y * CHUNK_SIZE, 0),
        glm::vec3((coord.x + 1) * CHUNK_SIZE, (coord.y + 1) * CHUNK_SIZE, 1)
    };
    
    // Check if bounds are in camera frustum
    return camera.getFrustum().intersects(bounds);
}

void WorldRenderer::render(const Camera& camera) {
    // ...
    for (const auto& chunk : chunks) {
        if (chunk->isVisibleFrom(camera)) {
            chunk->render();
        }
    }
    // ...
}
```

## Multithreading Optimizations

### Thread Pool Implementation

```cpp
class ThreadPool {
private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    
    std::mutex queueMutex;
    std::condition_variable condition;
    bool stop;
    
public:
    ThreadPool(size_t threads) : stop(false) {
        for (size_t i = 0; i < threads; ++i) {
            workers.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    
                    {
                        std::unique_lock<std::mutex> lock(queueMutex);
                        condition.wait(lock, [this] { 
                            return stop || !tasks.empty(); 
                        });
                        
                        if (stop && tasks.empty()) return;
                        
                        task = std::move(tasks.front());
                        tasks.pop();
                    }
                    
                    task();
                }
            });
        }
    }
    
    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<decltype(f(args...))> {
        using return_type = decltype(f(args...));
        
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        
        std::future<return_type> result = task->get_future();
        
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            if (stop) throw std::runtime_error("enqueue on stopped ThreadPool");
            tasks.emplace([task]() { (*task)(); });
        }
        
        condition.notify_one();
        return result;
    }
    
    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            stop = true;
        }
        
        condition.notify_all();
        
        for (std::thread& worker : workers) {
            worker.join();
        }
    }
};
```

### Parallel For Implementation

```cpp
template<typename Index, typename Function>
void parallelFor(Index start, Index end, Function fn, ThreadPool& pool) {
    const Index range = end - start;
    const Index numThreads = pool.size();
    
    if (range < numThreads) {
        // Too small to parallelize effectively
        for (Index i = start; i < end; ++i) {
            fn(i);
        }
        return;
    }
    
    // Split work among threads
    const Index chunkSize = range / numThreads;
    std::vector<std::future<void>> futures;
    
    for (Index t = 0; t < numThreads; ++t) {
        Index chunkStart = start + t * chunkSize;
        Index chunkEnd = (t == numThreads - 1) ? end : chunkStart + chunkSize;
        
        futures.push_back(pool.enqueue([chunkStart, chunkEnd, &fn] {
            for (Index i = chunkStart; i < chunkEnd; ++i) {
                fn(i);
            }
        }));
    }
    
    // Wait for all tasks to complete
    for (auto& future : futures) {
        future.wait();
    }
}
```

### Lock-Free Data Structures

Example of a lock-free queue for inter-thread communication:

```cpp
template<typename T>
class LockFreeQueue {
private:
    struct Node {
        std::shared_ptr<T> data;
        std::atomic<Node*> next;
        
        Node() : next(nullptr) {}
    };
    
    std::atomic<Node*> head;
    std::atomic<Node*> tail;
    
public:
    LockFreeQueue() {
        Node* dummy = new Node();
        head.store(dummy);
        tail.store(dummy);
    }
    
    ~LockFreeQueue() {
        while (Node* oldHead = head.load()) {
            head.store(oldHead->next);
            delete oldHead;
        }
    }
    
    void push(T value) {
        std::shared_ptr<T> newData = std::make_shared<T>(std::move(value));
        Node* newNode = new Node();
        newNode->data = newData;
        
        while (true) {
            Node* oldTail = tail.load();
            Node* next = oldTail->next.load();
            
            if (oldTail == tail.load()) {
                if (next == nullptr) {
                    if (oldTail->next.compare_exchange_weak(next, newNode)) {
                        tail.compare_exchange_weak(oldTail, newNode);
                        return;
                    }
                } else {
                    tail.compare_exchange_weak(oldTail, next);
                }
            }
        }
    }
    
    bool pop(T& value) {
        while (true) {
            Node* oldHead = head.load();
            Node* oldTail = tail.load();
            Node* next = oldHead->next.load();
            
            if (oldHead == head.load()) {
                if (oldHead == oldTail) {
                    if (next == nullptr) {
                        // Queue is empty
                        return false;
                    }
                    tail.compare_exchange_weak(oldTail, next);
                } else {
                    if (next == nullptr) {
                        continue;
                    }
                    
                    value = std::move(*next->data);
                    
                    if (head.compare_exchange_weak(oldHead, next)) {
                        delete oldHead;
                        return true;
                    }
                }
            }
        }
    }
};
```

## SIMD Optimizations

### SIMD for Physical Calculations

Use SIMD to accelerate physics calculations:

```cpp
void updateTemperatures(float* temperatures, int count) {
    // Standard implementation
    for (int i = 0; i < count; ++i) {
        temperatures[i] = temperatures[i] * 0.99f + 20.0f * 0.01f;
    }
}

void updateTemperaturesSIMD(float* temperatures, int count) {
    // Using SSE
    __m128 decay = _mm_set1_ps(0.99f);
    __m128 ambient = _mm_set1_ps(20.0f * 0.01f);
    
    int i = 0;
    for (; i <= count - 4; i += 4) {
        __m128 temp = _mm_loadu_ps(&temperatures[i]);
        __m128 result = _mm_add_ps(_mm_mul_ps(temp, decay), ambient);
        _mm_storeu_ps(&temperatures[i], result);
    }
    
    // Handle remaining elements
    for (; i < count; ++i) {
        temperatures[i] = temperatures[i] * 0.99f + 20.0f * 0.01f;
    }
}
```

### Vectorized Material Operations

SIMD for updating multiple cells of the same material type:

```cpp
void updateLiquidCellsBatch(Cell* cells, int* indices, int count) {
    // Load common constants
    __m128 gravity = _mm_set1_ps(9.8f);
    __m128 viscosity = _mm_set1_ps(0.5f);
    
    for (int i = 0; i < count; i += 4) {
        int batchSize = std::min(4, count - i);
        
        // Load cell data
        __m128 velocityY[4];
        for (int j = 0; j < batchSize; ++j) {
            velocityY[j] = _mm_set1_ps(cells[indices[i+j]].velocity.y);
        }
        
        // Update velocities with SIMD
        for (int j = 0; j < batchSize; ++j) {
            velocityY[j] = _mm_add_ps(velocityY[j], 
                           _mm_mul_ps(gravity, 
                           _mm_sub_ps(_mm_set1_ps(1.0f), viscosity)));
        }
        
        // Store updated data
        for (int j = 0; j < batchSize; ++j) {
            float result[4];
            _mm_storeu_ps(result, velocityY[j]);
            cells[indices[i+j]].velocity.y = result[0];
        }
    }
}
```

## GPU Acceleration

### Compute Shader for Fluid Simulation

```glsl
#version 430 core

layout(local_size_x = 16, local_size_y = 16) in;

// Input buffers
layout(std430, binding = 0) buffer MaterialBuffer {
    int materials[];
};

layout(std430, binding = 1) buffer VelocityBuffer {
    vec2 velocities[];
};

layout(std430, binding = 2) buffer TemperatureBuffer {
    float temperatures[];
};

// Output buffers
layout(std430, binding = 3) buffer NewMaterialBuffer {
    int newMaterials[];
};

layout(std430, binding = 4) buffer NewVelocityBuffer {
    vec2 newVelocities[];
};

layout(std430, binding = 5) buffer NewTemperatureBuffer {
    float newTemperatures[];
};

// Uniforms
uniform int width;
uniform int height;
uniform float deltaTime;
uniform vec2 gravity;

// Material constants
const int MATERIAL_AIR = 0;
const int MATERIAL_WATER = 1;
const int MATERIAL_SAND = 2;
// ... other materials

int getIndex(ivec2 pos) {
    return pos.y * width + pos.x;
}

bool isInBounds(ivec2 pos) {
    return pos.x >= 0 && pos.x < width && pos.y >= 0 && pos.y < height;
}

void main() {
    ivec2 pos = ivec2(gl_GlobalInvocationID.xy);
    
    if (!isInBounds(pos)) return;
    
    int index = getIndex(pos);
    int material = materials[index];
    vec2 velocity = velocities[index];
    float temperature = temperatures[index];
    
    // Start with current values
    int newMaterial = material;
    vec2 newVelocity = velocity;
    float newTemperature = temperature;
    
    // Apply physics based on material type
    if (material == MATERIAL_WATER) {
        // Water simulation
        
        // Apply gravity
        newVelocity += gravity * deltaTime;
        
        // Check below
        ivec2 belowPos = pos + ivec2(0, 1);
        if (isInBounds(belowPos)) {
            int belowIndex = getIndex(belowPos);
            int belowMaterial = materials[belowIndex];
            
            if (belowMaterial == MATERIAL_AIR) {
                // Move down
                newMaterials[belowIndex] = MATERIAL_WATER;
                newMaterials[index] = MATERIAL_AIR;
                newVelocities[belowIndex] = newVelocity;
                newTemperatures[belowIndex] = newTemperature;
                return;
            }
        }
        
        // Check diagonals and sides if can't move down
        // ...
    }
    else if (material == MATERIAL_SAND) {
        // Sand simulation
        // ...
    }
    
    // Store results if we haven't returned yet
    newMaterials[index] = newMaterial;
    newVelocities[index] = newVelocity;
    newTemperatures[index] = newTemperature;
}
```

### GPU Implementation Class

```cpp
class GPUPhysicsSimulator {
private:
    // Shader program
    GLuint computeProgram;
    
    // Buffers
    GLuint materialBuffer;
    GLuint velocityBuffer;
    GLuint temperatureBuffer;
    GLuint newMaterialBuffer;
    GLuint newVelocityBuffer;
    GLuint newTemperatureBuffer;
    
    // Dimensions
    int width;
    int height;
    
    // Data
    std::vector<int> materials;
    std::vector<glm::vec2> velocities;
    std::vector<float> temperatures;
    
public:
    GPUPhysicsSimulator(int width, int height);
    ~GPUPhysicsSimulator();
    
    void initialize();
    void update(float deltaTime);
    void readback();
    
    // Data access
    int getMaterial(int x, int y) const;
    void setMaterial(int x, int y, int material);
    
    glm::vec2 getVelocity(int x, int y) const;
    void setVelocity(int x, int y, const glm::vec2& velocity);
    
    float getTemperature(int x, int y) const;
    void setTemperature(int x, int y, float temperature);
};
```

## Profiling and Performance Analysis

### Custom Profiler Implementation

```cpp
class ScopedProfiler {
private:
    const char* name;
    std::chrono::high_resolution_clock::time_point startTime;
    
public:
    ScopedProfiler(const char* name) : name(name) {
        startTime = std::chrono::high_resolution_clock::now();
    }
    
    ~ScopedProfiler() {
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            endTime - startTime).count() / 1000.0f;
        
        std::cout << name << ": " << duration << "ms" << std::endl;
    }
};

#define PROFILE_SCOPE(name) ScopedProfiler profiler##__LINE__(name)
#define PROFILE_FUNCTION() ScopedProfiler profiler##__LINE__(__FUNCTION__)
```

### Performance Testing Framework

```cpp
class PerformanceTest {
private:
    std::string name;
    std::function<void()> testFunction;
    
public:
    PerformanceTest(const std::string& name, std::function<void()> testFunction)
        : name(name), testFunction(testFunction) {}
    
    void run(int iterations = 100) {
        // Warmup
        for (int i = 0; i < 10; ++i) {
            testFunction();
        }
        
        // Actual test
        auto startTime = std::chrono::high_resolution_clock::now();
        
        for (int i = 0; i < iterations; ++i) {
            testFunction();
        }
        
        auto endTime = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            endTime - startTime).count() / 1000.0f / iterations;
        
        std::cout << "Performance test '" << name << "': " 
                  << duration << "ms per iteration" << std::endl;
    }
};
```

## Conclusion

The Astral engine employs a wide range of optimization techniques to achieve high performance in cellular physics simulation. By combining memory optimizations, physics-specific optimizations, rendering optimizations, multithreading, SIMD, and GPU acceleration, the engine can handle large world sizes with complex physics behaviors while maintaining high frame rates.

Key optimization strategies include:

1. **Active cell tracking** to focus computation where it's needed
2. **Chunking** for efficient memory management and parallelization
3. **Data-oriented design** for cache efficiency
4. **Multithreading** with lock-free data structures
5. **SIMD instructions** for vectorized processing
6. **GPU acceleration** for parallel physics computation
7. **Instanced rendering** for efficient visualization
8. **Memory pooling** to reduce allocation overhead

These techniques are applied with careful profiling and benchmarking to ensure they provide meaningful performance improvements for the specific workloads encountered in cellular physics simulation.