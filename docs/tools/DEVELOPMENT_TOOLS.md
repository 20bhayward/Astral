# Development Tools

This document details the custom development tools we'll build to support the development and testing of the Astral engine. These tools are critical for ensuring a smooth development process, effective debugging, and maintaining high-quality code.

## Core Development Tools

### 1. AstralProfiler

A comprehensive profiling system for measuring performance across all engine subsystems.

```cpp
class ProfilerMarker {
private:
    std::string name;
    std::chrono::high_resolution_clock::time_point startTime;
    bool active;
    ProfilerCategory category;
    
public:
    ProfilerMarker(const std::string& markerName, ProfilerCategory category = ProfilerCategory::Default);
    ~ProfilerMarker(); // Automatically ends the marker
    
    void end();
};

class ThreadProfiler {
private:
    std::string threadName;
    std::vector<ProfilerEvent> events;
    std::stack<ProfilerEvent*> activeMarkers;
    
public:
    ThreadProfiler(const std::string& name);
    
    void beginMarker(const std::string& name, ProfilerCategory category);
    void endMarker(const std::string& name);
    
    const std::vector<ProfilerEvent>& getEvents() const;
    void clear();
};

class AstralProfiler {
private:
    static AstralProfiler* instance;
    
    bool enabled;
    std::unordered_map<std::thread::id, std::unique_ptr<ThreadProfiler>> threadProfilers;
    std::mutex profilerMutex;
    
    std::ofstream fileOutput;
    bool fileLoggingEnabled;
    
    // Performance data
    struct FrameData {
        float frameTime;
        float physicsTime;
        float renderTime;
        float uiTime;
        int activeChunks;
        int renderedCells;
        int updatedCells;
        size_t memoryUsage;
    };
    
    std::vector<FrameData> frameHistory;
    size_t frameHistoryMaxSize;
    
    AstralProfiler(); // Private constructor for singleton
    
public:
    static AstralProfiler* getInstance();
    
    void initialize(bool enableProfiling = true);
    void shutdown();
    
    void beginFrame();
    void endFrame();
    
    void beginMarker(const std::string& name, ProfilerCategory category = ProfilerCategory::Default);
    void endMarker(const std::string& name);
    
    // Convenience macros/functions
    ProfilerMarker createMarker(const std::string& name, ProfilerCategory category = ProfilerCategory::Default);
    
    // Enable/disable profiling at runtime
    void setEnabled(bool enable);
    bool isEnabled() const;
    
    // File output
    void enableFileLogging(const std::string& filePath);
    void disableFileLogging();
    
    // Memory tracking
    void trackAllocation(size_t size);
    void trackDeallocation(size_t size);
    
    // Frame data access
    const std::vector<FrameData>& getFrameHistory() const;
    FrameData getLastFrameData() const;
    float getAverageFrameRate() const;
    
    // Visualization helpers
    void renderImGuiWindow();
    void exportChrome(const std::string& filePath);
};

// Convenience macros for scoped profiling
#define ASTRAL_PROFILE_FUNCTION() auto __profileMarker = AstralProfiler::getInstance()->createMarker(__FUNCTION__, ProfilerCategory::Default)
#define ASTRAL_PROFILE_SCOPE(name, category) auto __profileMarker##__LINE__ = AstralProfiler::getInstance()->createMarker(name, category)
```

Features:
- Hierarchical timing system with markers
- Memory allocation tracking
- Thread-specific profiling
- Chrome Tracing format export
- ImGui integration for real-time visualization
- Frame history for trend analysis
- Category-based filtering

### 2. AstralDebugDraw

A visualization system for debugging physics, rendering, and other systems.

```cpp
enum class DebugDrawFlags {
    None = 0,
    Wireframe = 1 << 0,
    Filled = 1 << 1,
    Transparent = 1 << 2,
    PersistentLines = 1 << 3,
    DepthTest = 1 << 4,
    WorldSpace = 1 << 5,
    ScreenSpace = 1 << 6,
};

class DebugDraw {
private:
    static DebugDraw* instance;
    
    struct DebugVertex {
        glm::vec3 position;
        glm::vec4 color;
    };
    
    struct DebugDrawCommand {
        std::vector<DebugVertex> vertices;
        GLenum primitiveType;
        DebugDrawFlags flags;
        float duration;
        float timeRemaining;
    };
    
    std::vector<DebugDrawCommand> commands;
    ShaderProgram* debugShader;
    
    GLuint vao;
    GLuint vbo;
    
    Camera* camera;
    bool enabled;
    
    DebugDraw(); // Private constructor for singleton
    
public:
    static DebugDraw* getInstance();
    
    void initialize(ShaderProgram* shader, Camera* camera);
    void shutdown();
    
    void setEnabled(bool enable);
    bool isEnabled() const;
    
    // Basic primitives
    void drawLine(const glm::vec3& start, const glm::vec3& end, const glm::vec4& color, float duration = 0.0f, DebugDrawFlags flags = DebugDrawFlags::WorldSpace);
    void drawRect(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color, float duration = 0.0f, DebugDrawFlags flags = DebugDrawFlags::WorldSpace);
    void drawCircle(const glm::vec2& center, float radius, const glm::vec4& color, int segments = 16, float duration = 0.0f, DebugDrawFlags flags = DebugDrawFlags::WorldSpace);
    void drawPolygon(const std::vector<glm::vec2>& points, const glm::vec4& color, float duration = 0.0f, DebugDrawFlags flags = DebugDrawFlags::WorldSpace);
    void drawText(const glm::vec2& position, const std::string& text, const glm::vec4& color, float scale = 1.0f, float duration = 0.0f, DebugDrawFlags flags = DebugDrawFlags::WorldSpace);
    
    // Physics visualization
    void drawChunkBoundaries(const ChunkManager* chunkManager, const glm::vec4& color, float duration = 0.0f);
    void drawActiveChunks(const ChunkManager* chunkManager, const glm::vec4& color, float duration = 0.0f);
    void drawCellVelocities(const ChunkManager* chunkManager, const MaterialRegistry* materials, float scale = 1.0f, float duration = 0.0f);
    void drawTemperatureMap(const ChunkManager* chunkManager, float duration = 0.0f);
    
    // Grid helpers
    void drawGrid(const glm::vec2& origin, float size, int divisions, const glm::vec4& color, float duration = 0.0f);
    
    // Update and render
    void update(float deltaTime);
    void render();
    
    // Clear all commands
    void clear();
};

// Helper macros
#define DEBUG_DRAW_LINE(start, end, color) if(DebugDraw::getInstance()->isEnabled()) DebugDraw::getInstance()->drawLine(start, end, color)
#define DEBUG_DRAW_RECT(pos, size, color) if(DebugDraw::getInstance()->isEnabled()) DebugDraw::getInstance()->drawRect(pos, size, color)
```

Features:
- Line, rectangle, circle, and polygon drawing
- Text rendering for labels and values
- Grid drawing for spatial reference
- Chunk boundary visualization
- Temperature and velocity field visualization
- Duration-based drawing for animations
- World and screen space coordinates

### 3. AstralConsole

An in-game console for executing commands and testing features.

```cpp
class ConsoleCommand {
private:
    std::string name;
    std::string description;
    std::function<void(const std::vector<std::string>&)> callback;
    
public:
    ConsoleCommand(const std::string& name, 
                  const std::string& description, 
                  std::function<void(const std::vector<std::string>&)> callback);
    
    const std::string& getName() const;
    const std::string& getDescription() const;
    void execute(const std::vector<std::string>& args);
};

class AstralConsole {
private:
    static AstralConsole* instance;
    
    bool visible;
    std::string inputBuffer;
    std::vector<std::string> history;
    std::vector<std::string> commandHistory;
    int historyPosition;
    
    std::unordered_map<std::string, ConsoleCommand> commands;
    
    AstralConsole(); // Private constructor for singleton
    
public:
    static AstralConsole* getInstance();
    
    void initialize();
    void shutdown();
    
    void registerCommand(const std::string& name, 
                        const std::string& description, 
                        std::function<void(const std::vector<std::string>&)> callback);
    
    void unregisterCommand(const std::string& name);
    
    void executeCommand(const std::string& commandLine);
    
    void log(const std::string& message);
    void warning(const std::string& message);
    void error(const std::string& message);
    
    void setVisible(bool visible);
    bool isVisible() const;
    bool toggleVisible();
    
    void renderImGui();
    
    bool handleKeyEvent(int key, int scancode, int action, int mods);
};

// Helper macros
#define CONSOLE_LOG(message) AstralConsole::getInstance()->log(message)
#define CONSOLE_WARNING(message) AstralConsole::getInstance()->warning(message)
#define CONSOLE_ERROR(message) AstralConsole::getInstance()->error(message)
```

Features:
- Command registration system
- Command history
- Argument parsing
- Tab completion
- Color-coded output
- Scrollable history
- Variable inspection
- Built-in help system

### 4. AstralInspector

A property inspector for examining and modifying engine objects at runtime.

```cpp
class InspectableProperty {
public:
    enum class Type {
        Int,
        Float,
        Bool,
        String,
        Vec2,
        Vec3,
        Vec4,
        Color,
        Enum,
        Object
    };
    
private:
    std::string name;
    std::string description;
    Type type;
    
    // Value accessors
    std::function<void*()> getter;
    std::function<void(void*)> setter;
    
    // For enum types
    std::vector<std::string> enumNames;
    
    // For object types
    std::function<void*()> objectGetter;
    
public:
    InspectableProperty(const std::string& name, 
                        const std::string& description, 
                        Type type,
                        std::function<void*()> getter,
                        std::function<void(void*)> setter = nullptr);
    
    // Constructor for enum types
    InspectableProperty(const std::string& name, 
                        const std::string& description, 
                        const std::vector<std::string>& enumNames,
                        std::function<int()> getter,
                        std::function<void(int)> setter = nullptr);
    
    // Constructor for object types
    InspectableProperty(const std::string& name, 
                        const std::string& description, 
                        std::function<void*()> objectGetter);
    
    const std::string& getName() const;
    const std::string& getDescription() const;
    Type getType() const;
    
    void* getValue();
    void setValue(void* value);
    
    // For enum types
    const std::vector<std::string>& getEnumNames() const;
    
    // For object types
    void* getObject();
    
    bool isReadOnly() const;
};

class Inspectable {
private:
    std::string name;
    std::vector<InspectableProperty> properties;
    
public:
    Inspectable(const std::string& name);
    
    void addProperty(const InspectableProperty& property);
    
    const std::string& getName() const;
    const std::vector<InspectableProperty>& getProperties() const;
};

class AstralInspector {
private:
    static AstralInspector* instance;
    
    bool visible;
    std::unordered_map<std::string, Inspectable> inspectables;
    std::string selectedObject;
    
    AstralInspector(); // Private constructor for singleton
    
public:
    static AstralInspector* getInstance();
    
    void initialize();
    void shutdown();
    
    void registerInspectable(const Inspectable& inspectable);
    void unregisterInspectable(const std::string& name);
    
    void setVisible(bool visible);
    bool isVisible() const;
    bool toggleVisible();
    
    void setSelectedObject(const std::string& name);
    const std::string& getSelectedObject() const;
    
    void renderImGui();
};
```

Features:
- Property editing for different types
- Real-time updates
- Type-safe property access
- Object hierarchy navigation
- Search functionality
- Custom property editors
- Value history tracking
- Undo/redo support

### 5. AstralTestBench

A sandbox for testing physics behaviors in isolation.

```cpp
class PhysicsScenario {
private:
    std::string name;
    std::string description;
    std::function<void(ChunkManager*, MaterialRegistry*)> setupFunction;
    
public:
    PhysicsScenario(const std::string& name, 
                   const std::string& description, 
                   std::function<void(ChunkManager*, MaterialRegistry*)> setupFunction);
    
    const std::string& getName() const;
    const std::string& getDescription() const;
    void setup(ChunkManager* chunkManager, MaterialRegistry* materials);
};

class TestParameterSet {
private:
    std::string name;
    std::unordered_map<std::string, float> floatParams;
    std::unordered_map<std::string, int> intParams;
    std::unordered_map<std::string, bool> boolParams;
    std::unordered_map<std::string, std::string> stringParams;
    
public:
    TestParameterSet(const std::string& name);
    
    void setFloat(const std::string& name, float value);
    void setInt(const std::string& name, int value);
    void setBool(const std::string& name, bool value);
    void setString(const std::string& name, const std::string& value);
    
    float getFloat(const std::string& name, float defaultValue = 0.0f) const;
    int getInt(const std::string& name, int defaultValue = 0) const;
    bool getBool(const std::string& name, bool defaultValue = false) const;
    std::string getString(const std::string& name, const std::string& defaultValue = "") const;
    
    void save(const std::string& filePath);
    void load(const std::string& filePath);
};

class AstralTestBench {
private:
    static AstralTestBench* instance;
    
    bool active;
    std::unordered_map<std::string, PhysicsScenario> scenarios;
    std::string currentScenario;
    
    // Test bench state
    ChunkManager* testChunkManager;
    MaterialRegistry* testMaterialRegistry;
    CellularPhysics* testPhysics;
    
    // Test parameters
    TestParameterSet currentParams;
    std::vector<TestParameterSet> savedParamSets;
    
    // Performance measurement
    AstralProfiler* profiler;
    std::vector<float> frameTimes;
    
    // Recording
    bool recording;
    std::vector<std::vector<Cell>> recordedFrames;
    
    AstralTestBench(); // Private constructor for singleton
    
public:
    static AstralTestBench* getInstance();
    
    void initialize(AstralProfiler* profiler);
    void shutdown();
    
    void registerScenario(const PhysicsScenario& scenario);
    void loadScenario(const std::string& name);
    
    void reset();
    void step(int numSteps = 1);
    void run(bool running = true);
    bool isRunning() const;
    
    void startRecording();
    void stopRecording();
    void playbackRecording(int frameRate = 60);
    void saveRecording(const std::string& filePath);
    void loadRecording(const std::string& filePath);
    
    void saveParameterSet(const std::string& name);
    void loadParameterSet(const std::string& name);
    
    void renderImGui();
};
```

Features:
- Predefined test scenarios
- Parameter tweaking
- Performance measurement
- Recording and playback
- Comparison mode for before/after changes
- Material property editor
- Step-by-step simulation
- Reset functionality

### 6. AstralScriptingEngine

A scripting system for automating tests and extending functionality.

```cpp
class ScriptEngine {
private:
    static ScriptEngine* instance;
    
    lua_State* L;
    bool initialized;
    
    // Script registry
    std::unordered_map<std::string, std::string> loadedScripts;
    
    // Error handling
    std::string lastError;
    
    ScriptEngine(); // Private constructor for singleton
    
public:
    static ScriptEngine* getInstance();
    
    bool initialize();
    void shutdown();
    
    // Script management
    bool loadScript(const std::string& name, const std::string& filePath);
    bool loadScriptFromString(const std::string& name, const std::string& scriptCode);
    bool unloadScript(const std::string& name);
    bool reloadScript(const std::string& name);
    
    // Function execution
    bool callFunction(const std::string& scriptName, const std::string& functionName);
    template<typename... Args>
    bool callFunction(const std::string& scriptName, const std::string& functionName, Args... args);
    
    // Global functions
    void registerGlobalFunction(const std::string& name, lua_CFunction function);
    
    // Object binding
    template<typename T>
    void registerClass(const std::string& className);
    
    template<typename T>
    void registerClassMethod(const std::string& className, const std::string& methodName, 
                            int (T::*method)(lua_State*));
    
    template<typename T>
    void pushObject(const std::string& className, T* object);
    
    // Error handling
    const std::string& getLastError() const;
    
    // Direct Lua state access (for advanced usage)
    lua_State* getLuaState();
};
```

Features:
- Lua integration
- Script hot-reloading
- Console command binding
- Engine API exposure
- Custom material behavior scripting
- Event system integration
- Automated testing
- Performance benchmarking

### 7. AstralMaterialEditor

A specialized tool for creating and editing materials.

```cpp
class MaterialProperty {
public:
    enum class Type {
        Float,
        Int,
        Bool,
        Color,
        Enum
    };
    
private:
    std::string name;
    Type type;
    void* data;
    
    // For enum types
    std::vector<std::string> enumNames;
    
public:
    MaterialProperty(const std::string& name, Type type, void* data);
    
    // Constructor for enum types
    MaterialProperty(const std::string& name, const std::vector<std::string>& enumNames, int* data);
    
    const std::string& getName() const;
    Type getType() const;
    void* getData() const;
    
    // For enum types
    const std::vector<std::string>& getEnumNames() const;
};

class MaterialTemplate {
private:
    std::string name;
    MaterialType type;
    std::vector<MaterialProperty> properties;
    
    // Preview
    GLuint previewTexture;
    
public:
    MaterialTemplate(const std::string& name, MaterialType type);
    ~MaterialTemplate();
    
    void addProperty(const MaterialProperty& property);
    
    const std::string& getName() const;
    MaterialType getType() const;
    const std::vector<MaterialProperty>& getProperties() const;
    
    void updatePreview();
    GLuint getPreviewTexture() const;
    
    MaterialProperties createMaterialProperties() const;
};

class MaterialEditor {
private:
    static MaterialEditor* instance;
    
    bool visible;
    std::vector<MaterialTemplate> templates;
    std::vector<MaterialProperties> customMaterials;
    
    // Current editing state
    MaterialProperties* currentMaterial;
    std::string currentMaterialName;
    
    // Undo/redo
    struct MaterialEditAction {
        std::string propertyName;
        void* oldValue;
        void* newValue;
        size_t valueSize;
    };
    
    std::vector<MaterialEditAction> undoStack;
    std::vector<MaterialEditAction> redoStack;
    
    MaterialEditor(); // Private constructor for singleton
    
public:
    static MaterialEditor* getInstance();
    
    void initialize();
    void shutdown();
    
    void registerTemplate(const MaterialTemplate& templ);
    
    void createMaterial(const std::string& name, const std::string& templateName);
    void editMaterial(const std::string& name);
    void deleteMaterial(const std::string& name);
    
    bool saveMaterial(const std::string& filePath);
    bool loadMaterial(const std::string& filePath);
    
    void applyMaterialToRegistry(MaterialRegistry* registry);
    
    void undo();
    void redo();
    
    void setVisible(bool visible);
    bool isVisible() const;
    bool toggleVisible();
    
    void renderImGui();
};
```

Features:
- Material property editing
- Visual preview
- Texture creation
- Reaction rule editor
- Material testing
- Template-based creation
- Import/export functionality
- Material libraries

### 8. AstralPerformanceBenchmark

A benchmarking system for consistent performance testing.

```cpp
struct BenchmarkResult {
    std::string name;
    double averageTime;
    double minTime;
    double maxTime;
    double standardDeviation;
    size_t memoryUsage;
    int cellCount;
    std::string timestamp;
    std::unordered_map<std::string, double> customMetrics;
};

class BenchmarkCase {
private:
    std::string name;
    std::string description;
    std::function<void(AstralEngine*)> setupFunction;
    std::function<void(AstralEngine*, float)> runFunction;
    std::function<bool(AstralEngine*)> validateFunction;
    float duration;
    int warmupFrames;
    
public:
    BenchmarkCase(const std::string& name, 
                 const std::string& description, 
                 std::function<void(AstralEngine*)> setupFunction,
                 std::function<void(AstralEngine*, float)> runFunction,
                 std::function<bool(AstralEngine*)> validateFunction,
                 float duration = 5.0f,
                 int warmupFrames = 60);
    
    const std::string& getName() const;
    const std::string& getDescription() const;
    float getDuration() const;
    int getWarmupFrames() const;
    
    void setup(AstralEngine* engine);
    void run(AstralEngine* engine, float deltaTime);
    bool validate(AstralEngine* engine);
};

class PerformanceBenchmark {
private:
    static PerformanceBenchmark* instance;
    
    std::unordered_map<std::string, BenchmarkCase> benchmarks;
    std::vector<BenchmarkResult> results;
    
    // Current benchmark state
    bool running;
    std::string currentBenchmark;
    float currentTime;
    int currentFrame;
    bool inWarmup;
    
    // Metrics collection
    std::vector<double> frameTimes;
    std::unordered_map<std::string, std::vector<double>> customMetrics;
    
    // Reference system specs
    struct SystemInfo {
        std::string cpuName;
        int coreCount;
        size_t totalMemory;
        std::string gpuName;
        std::string osName;
        std::string buildVersion;
    } systemInfo;
    
    PerformanceBenchmark(); // Private constructor for singleton
    
public:
    static PerformanceBenchmark* getInstance();
    
    void initialize();
    void shutdown();
    
    void registerBenchmark(const BenchmarkCase& benchmark);
    
    void runBenchmark(const std::string& name);
    void runAllBenchmarks();
    
    void update(AstralEngine* engine, float deltaTime);
    
    void trackCustomMetric(const std::string& name, double value);
    
    bool isRunning() const;
    
    const std::vector<BenchmarkResult>& getResults() const;
    
    void saveResults(const std::string& filePath);
    void loadResults(const std::string& filePath);
    
    void compareResults(const std::string& resultFile1, const std::string& resultFile2);
    
    void renderImGui();
};
```

Features:
- Standardized benchmark cases
- Performance regression detection
- Memory usage tracking
- CPU and GPU profiling
- Comparative analysis
- Custom metric tracking
- System information collection
- Report generation

### 9. AstralPhysicsVisualizer

A specialized tool for visualizing physics behavior.

```cpp
enum class VisualizationMode {
    Normal,
    Velocity,
    Temperature,
    Pressure,
    Density,
    Material,
    ActiveCells
};

class PhysicsVisualizer {
private:
    static PhysicsVisualizer* instance;
    
    bool enabled;
    VisualizationMode currentMode;
    ShaderProgram* visualizationShader;
    
    // Visualization parameters
    float velocityScale;
    float temperatureMin;
    float temperatureMax;
    float pressureScale;
    
    // Color maps
    GLuint colorMapTexture;
    std::unordered_map<std::string, int> colorMaps;
    std::string currentColorMap;
    
    // Animation
    bool animating;
    float animationTime;
    
    PhysicsVisualizer(); // Private constructor for singleton
    
public:
    static PhysicsVisualizer* getInstance();
    
    void initialize(ShaderProgram* shader);
    void shutdown();
    
    void setEnabled(bool enable);
    bool isEnabled() const;
    
    void setMode(VisualizationMode mode);
    VisualizationMode getMode() const;
    
    void setColorMap(const std::string& name);
    const std::string& getColorMap() const;
    
    // Parameter setters
    void setVelocityScale(float scale);
    void setTemperatureRange(float min, float max);
    void setPressureScale(float scale);
    
    // Animation control
    void setAnimating(bool animate);
    bool isAnimating() const;
    
    void update(float deltaTime);
    void render(const ChunkManager* chunkManager, const MaterialRegistry* materials);
    
    void renderImGui();
};
```

Features:
- Multiple visualization modes
- Color mapping
- Vector field visualization
- Heat map rendering
- Animation of changes
- Cross-section view
- Time-lapse recording
- Particle tracing

### 10. AstralLogger

A comprehensive logging system for tracking engine activity.

```cpp
enum class LogLevel {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Critical
};

class LogSink {
public:
    virtual ~LogSink() = default;
    
    virtual void log(LogLevel level, const std::string& message) = 0;
    virtual void flush() = 0;
};

class ConsoleSink : public LogSink {
public:
    void log(LogLevel level, const std::string& message) override;
    void flush() override;
};

class FileSink : public LogSink {
private:
    std::ofstream file;
    std::string filePath;
    
public:
    FileSink(const std::string& filePath);
    ~FileSink();
    
    void log(LogLevel level, const std::string& message) override;
    void flush() override;
};

class AstralLogger {
private:
    static AstralLogger* instance;
    
    LogLevel minLevel;
    std::vector<std::unique_ptr<LogSink>> sinks;
    std::mutex logMutex;
    
    // Logging state
    bool initialized;
    bool enableTimestamps;
    bool enableColors;
    
    std::unordered_map<std::string, LogLevel> categoryLevels;
    
    AstralLogger(); // Private constructor for singleton
    
public:
    static AstralLogger* getInstance();
    
    void initialize();
    void shutdown();
    
    void addSink(std::unique_ptr<LogSink> sink);
    
    void setMinLevel(LogLevel level);
    LogLevel getMinLevel() const;
    
    void setCategoryLevel(const std::string& category, LogLevel level);
    
    void setEnableTimestamps(bool enable);
    void setEnableColors(bool enable);
    
    void log(LogLevel level, const std::string& category, const std::string& message);
    
    void trace(const std::string& category, const std::string& message);
    void debug(const std::string& category, const std::string& message);
    void info(const std::string& category, const std::string& message);
    void warning(const std::string& category, const std::string& message);
    void error(const std::string& category, const std::string& message);
    void critical(const std::string& category, const std::string& message);
    
    void flush();
    
    void renderImGui();
};

// Helper macros
#define LOG_TRACE(category, message) AstralLogger::getInstance()->trace(category, message)
#define LOG_DEBUG(category, message) AstralLogger::getInstance()->debug(category, message)
#define LOG_INFO(category, message) AstralLogger::getInstance()->info(category, message)
#define LOG_WARNING(category, message) AstralLogger::getInstance()->warning(category, message)
#define LOG_ERROR(category, message) AstralLogger::getInstance()->error(category, message)
#define LOG_CRITICAL(category, message) AstralLogger::getInstance()->critical(category, message)
```

Features:
- Multiple log sinks (console, file, custom)
- Log levels
- Category-based filtering
- Thread-safe logging
- Formatted output
- Timestamp and context information
- In-game log viewer
- Log rotation
- Performance-optimized logging

## Integration and Test Tools

### 1. AstralTestFramework

An automated testing system for engine components.

```cpp
class TestCase {
private:
    std::string name;
    std::string category;
    std::function<bool()> testFunction;
    
public:
    TestCase(const std::string& name, 
            const std::string& category, 
            std::function<bool()> testFunction);
    
    const std::string& getName() const;
    const std::string& getCategory() const;
    bool run();
};

class TestSuite {
private:
    std::string name;
    std::vector<TestCase> tests;
    
public:
    TestSuite(const std::string& name);
    
    void addTest(const TestCase& test);
    
    const std::string& getName() const;
    const std::vector<TestCase>& getTests() const;
    
    int runAll();
};

class TestResult {
public:
    std::string testName;
    std::string category;
    bool success;
    std::string errorMessage;
    double duration;
};

class TestFramework {
private:
    static TestFramework* instance;
    
    std::vector<TestSuite> suites;
    std::vector<TestResult> results;
    
    TestFramework(); // Private constructor for singleton
    
public:
    static TestFramework* getInstance();
    
    void initialize();
    void shutdown();
    
    void registerSuite(const TestSuite& suite);
    
    int runAllTests();
    int runSuite(const std::string& suiteName);
    int runCategory(const std::string& category);
    
    const std::vector<TestResult>& getResults() const;
    
    void clearResults();
    
    void generateReport(const std::string& filePath);
    
    void renderImGui();
};

// Helper macros
#define TEST_ASSERT(condition) if(!(condition)) { return false; }
#define TEST_ASSERT_EQUALS(a, b) if((a) != (b)) { return false; }
#define TEST_ASSERT_NEAR(a, b, epsilon) if(std::abs((a) - (b)) > (epsilon)) { return false; }
```

Features:
- Unit test framework
- Integration test support
- Regression testing
- Test categories
- Test suites
- Automated test runs
- Result reporting
- Visual test runner

### 2. AstralSceneRecorder

A system for recording and replaying simulation states.

```cpp
struct SimulationFrame {
    float timestamp;
    std::vector<Chunk> chunks;
    std::unordered_map<std::string, std::string> metadata;
};

class SceneRecorder {
private:
    static SceneRecorder* instance;
    
    bool recording;
    bool playing;
    
    std::string recordingName;
    std::vector<SimulationFrame> frames;
    
    // Recording state
    float recordingStartTime;
    float recordingTime;
    float recordingInterval;
    
    // Playback state
    size_t currentFrame;
    float playbackSpeed;
    bool looping;
    
    ChunkManager* chunkManager;
    
    SceneRecorder(); // Private constructor for singleton
    
public:
    static SceneRecorder* getInstance();
    
    void initialize(ChunkManager* chunkManager);
    void shutdown();
    
    void startRecording(const std::string& name, float interval = 1.0f / 30.0f);
    void stopRecording();
    bool isRecording() const;
    
    void captureFrame();
    
    void startPlayback(float speed = 1.0f, bool loop = false);
    void pausePlayback();
    void stopPlayback();
    void setPlaybackFrame(size_t frame);
    bool isPlaying() const;
    
    void update(float deltaTime);
    
    bool saveRecording(const std::string& filePath);
    bool loadRecording(const std::string& filePath);
    
    size_t getFrameCount() const;
    size_t getCurrentFrame() const;
    
    void setMetadata(const std::string& key, const std::string& value);
    std::string getMetadata(const std::string& key) const;
    
    void renderImGui();
};
```

Features:
- State recording
- Playback with speed control
- Frame-by-frame stepping
- Recording compression
- Metadata storage
- Comparison mode
- Recording editing
- Export to video

### 3. AstralScenarioBenchmark

A tool for benchmarking specific scenarios.

```cpp
struct ScenarioParameters {
    int worldWidth;
    int worldHeight;
    int cellCount;
    int materialCount;
    float simulationDuration;
    
    // Material-specific parameters
    std::unordered_map<std::string, float> materialParams;
    
    // Physics parameters
    float gravity;
    float viscosity;
    float temperature;
    
    // Rendering parameters
    bool enableRendering;
    bool enablePostProcessing;
    
    // Threading parameters
    int threadCount;
    bool enableGPU;
};

struct ScenarioResult {
    double averageFrameTime;
    double averagePhysicsTime;
    double averageRenderTime;
    
    int frameCount;
    int cellUpdates;
    double cellUpdatesPerSecond;
    
    size_t peakMemoryUsage;
    
    // Material-specific metrics
    std::unordered_map<std::string, double> materialMetrics;
};

class BenchmarkScenario {
private:
    std::string name;
    std::string description;
    
    // Setup/teardown functions
    std::function<void(AstralEngine*, const ScenarioParameters&)> setupFunction;
    std::function<void(AstralEngine*)> teardownFunction;
    
    // Default parameters
    ScenarioParameters defaultParams;
    
public:
    BenchmarkScenario(const std::string& name, 
                      const std::string& description, 
                      std::function<void(AstralEngine*, const ScenarioParameters&)> setupFunction,
                      std::function<void(AstralEngine*)> teardownFunction,
                      const ScenarioParameters& defaultParams);
    
    const std::string& getName() const;
    const std::string& getDescription() const;
    const ScenarioParameters& getDefaultParameters() const;
    
    void setup(AstralEngine* engine, const ScenarioParameters& params);
    void teardown(AstralEngine* engine);
};

class ScenarioBenchmark {
private:
    static ScenarioBenchmark* instance;
    
    std::unordered_map<std::string, BenchmarkScenario> scenarios;
    std::vector<ScenarioResult> results;
    
    // Current benchmark state
    bool running;
    std::string currentScenario;
    ScenarioParameters currentParams;
    
    float currentTime;
    float totalTime;
    
    AstralEngine* engine;
    
    ScenarioBenchmark(); // Private constructor for singleton
    
public:
    static ScenarioBenchmark* getInstance();
    
    void initialize(AstralEngine* engine);
    void shutdown();
    
    void registerScenario(const BenchmarkScenario& scenario);
    
    void runScenario(const std::string& name, const ScenarioParameters& params);
    void runAllScenarios();
    
    void update(float deltaTime);
    
    bool isRunning() const;
    
    const std::vector<ScenarioResult>& getResults() const;
    ScenarioResult getLastResult() const;
    
    void saveResults(const std::string& filePath);
    void loadResults(const std::string& filePath);
    
    void compareResults(const std::string& resultFile1, const std::string& resultFile2);
    
    void renderImGui();
};
```

Features:
- Predefined benchmark scenarios
- Parameter customization
- Performance metric collection
- Comparative analysis
- Result visualization
- Hardware-specific results
- Parallel scenario execution
- Report generation

### 4. AstralConfigurationTool

A tool for managing and tuning engine configuration.

```cpp
enum class ConfigValueType {
    Int,
    Float,
    Bool,
    String,
    Vector2,
    Vector3,
    Vector4
};

class ConfigValue {
private:
    std::string key;
    ConfigValueType type;
    std::variant<int, float, bool, std::string, glm::vec2, glm::vec3, glm::vec4> value;
    
    // Optional metadata
    std::string description;
    std::variant<int, float, bool, std::string, glm::vec2, glm::vec3, glm::vec4> defaultValue;
    std::variant<int, float, bool, std::string, glm::vec2, glm::vec3, glm::vec4> minValue;
    std::variant<int, float, bool, std::string, glm::vec2, glm::vec3, glm::vec4> maxValue;
    
public:
    template<typename T>
    ConfigValue(const std::string& key, T value, const std::string& description = "");
    
    const std::string& getKey() const;
    ConfigValueType getType() const;
    
    template<typename T>
    T getValue() const;
    
    template<typename T>
    void setValue(T value);
    
    const std::string& getDescription() const;
    
    template<typename T>
    void setRange(T min, T max);
    
    template<typename T>
    T getMinValue() const;
    
    template<typename T>
    T getMaxValue() const;
    
    template<typename T>
    T getDefaultValue() const;
};

class ConfigurationTool {
private:
    static ConfigurationTool* instance;
    
    bool visible;
    std::unordered_map<std::string, ConfigValue> config;
    
    // Configuration groups
    std::unordered_map<std::string, std::vector<std::string>> groups;
    
    // Change tracking
    bool changed;
    std::unordered_set<std::string> changedKeys;
    
    // Presets
    std::unordered_map<std::string, std::unordered_map<std::string, ConfigValue>> presets;
    
    ConfigurationTool(); // Private constructor for singleton
    
public:
    static ConfigurationTool* getInstance();
    
    void initialize();
    void shutdown();
    
    template<typename T>
    void registerValue(const std::string& key, T defaultValue, const std::string& group, const std::string& description = "");
    
    template<typename T>
    void setRange(const std::string& key, T min, T max);
    
    template<typename T>
    T getValue(const std::string& key, T defaultValue) const;
    
    template<typename T>
    void setValue(const std::string& key, T value);
    
    void registerGroup(const std::string& name, const std::vector<std::string>& keys);
    
    void savePreset(const std::string& name);
    void loadPreset(const std::string& name);
    void deletePreset(const std::string& name);
    
    bool hasChanged() const;
    const std::unordered_set<std::string>& getChangedKeys() const;
    void clearChangedState();
    
    bool saveToFile(const std::string& filePath);
    bool loadFromFile(const std::string& filePath);
    
    void setVisible(bool visible);
    bool isVisible() const;
    bool toggleVisible();
    
    void renderImGui();
};
```

Features:
- Configuration editing
- Type-safe values
- Configuration groups
- Presets
- Value validation
- Range constraints
- Configuration comparison
- Import/export functionality
- Change tracking

## Build and Development Tools

### 1. AstralBuildTool

A command-line tool for managing builds and project setup.

```cpp
enum class BuildType {
    Debug,
    Release,
    Profile,
    Distribution
};

enum class BuildPlatform {
    Windows,
    MacOS,
    Linux,
    Android,
    iOS
};

struct BuildConfig {
    BuildType type;
    BuildPlatform platform;
    bool enableTests;
    bool enableProfiler;
    bool enableTools;
    bool enableAssertions;
    bool enableValidation;
    
    // Compiler options
    std::string compiler;
    std::vector<std::string> compilerFlags;
    std::vector<std::string> linkerFlags;
    
    // Optimization options
    bool enableOptimizations;
    bool enableLTO;
    int optimizationLevel;
    
    // Advanced options
    bool enableSanitizers;
    bool enableCoverage;
    bool enablePGO;
};

class BuildTool {
private:
    BuildConfig config;
    std::string projectRoot;
    std::string buildDirectory;
    
    // Build state
    bool buildInProgress;
    int buildProgress;
    std::string currentTask;
    
    // Build results
    bool lastBuildSuccess;
    std::string buildLog;
    std::vector<std::string> warnings;
    std::vector<std::string> errors;
    
public:
    BuildTool(const std::string& projectRoot);
    
    void setConfig(const BuildConfig& config);
    const BuildConfig& getConfig() const;
    
    bool configure();
    bool build();
    bool clean();
    bool rebuild();
    
    bool runTests();
    bool package();
    bool install();
    
    // Project management
    bool createProject(const std::string& name, const std::string& location);
    bool importProject(const std::string& path);
    
    // Dependency management
    bool installDependency(const std::string& name, const std::string& version);
    bool updateDependencies();
    
    // Build status
    bool isBuildInProgress() const;
    int getBuildProgress() const;
    const std::string& getCurrentTask() const;
    
    bool wasLastBuildSuccessful() const;
    const std::string& getBuildLog() const;
    const std::vector<std::string>& getWarnings() const;
    const std::vector<std::string>& getErrors() const;
    
    // Utility
    void generateCompileCommands();
    void analyzeCode();
    
    void renderImGui();
};
```

Features:
- Multiple build configuration support
- Dependency management
- Project creation and import
- Test execution
- Packaging and distribution
- Code analysis integration
- Automated validation
- Project structure generation

### 2. AstralAssetPipeline

A tool for managing and processing game assets.

```cpp
enum class AssetType {
    Texture,
    Sound,
    Model,
    Material,
    Shader,
    Font,
    Script,
    Data
};

struct AssetMetadata {
    std::string id;
    AssetType type;
    std::string sourcePath;
    std::string outputPath;
    
    // Common properties
    bool compressed;
    
    // Type-specific properties
    union {
        struct {
            int width;
            int height;
            int channels;
            bool mipmaps;
            bool sRGB;
        } texture;
        
        struct {
            float duration;
            int sampleRate;
            int channels;
        } sound;
        
        struct {
            int vertexCount;
            int triangleCount;
            bool animated;
        } model;
    };
    
    std::unordered_map<std::string, std::string> customProperties;
};

class AssetProcessor {
public:
    virtual ~AssetProcessor() = default;
    
    virtual bool process(const std::string& sourcePath, const std::string& outputPath, const AssetMetadata& metadata) = 0;
};

class TextureProcessor : public AssetProcessor {
public:
    bool process(const std::string& sourcePath, const std::string& outputPath, const AssetMetadata& metadata) override;
};

class SoundProcessor : public AssetProcessor {
public:
    bool process(const std::string& sourcePath, const std::string& outputPath, const AssetMetadata& metadata) override;
};

class AssetPipeline {
private:
    static AssetPipeline* instance;
    
    std::string assetSourceDirectory;
    std::string assetOutputDirectory;
    
    std::unordered_map<AssetType, std::unique_ptr<AssetProcessor>> processors;
    std::unordered_map<std::string, AssetMetadata> assetRegistry;
    
    // Processing state
    bool processing;
    int processedAssets;
    int totalAssets;
    std::string currentAsset;
    
    // Watch state
    bool watching;
    std::vector<std::string> changedFiles;
    
    AssetPipeline(); // Private constructor for singleton
    
public:
    static AssetPipeline* getInstance();
    
    void initialize(const std::string& sourceDir, const std::string& outputDir);
    void shutdown();
    
    void registerProcessor(AssetType type, std::unique_ptr<AssetProcessor> processor);
    
    bool registerAsset(const AssetMetadata& metadata);
    bool unregisterAsset(const std::string& id);
    
    bool processAsset(const std::string& id);
    bool processAllAssets();
    bool processChangedAssets();
    
    void startWatching();
    void stopWatching();
    bool isWatching() const;
    
    bool isProcessing() const;
    int getProcessProgress() const;
    const std::string& getCurrentAsset() const;
    
    bool saveAssetRegistry(const std::string& filePath);
    bool loadAssetRegistry(const std::string& filePath);
    
    void renderImGui();
};
```

Features:
- Asset processing pipeline
- Automatic asset conversion
- Asset dependency tracking
- File watching for live updates
- Batch processing
- Asset metadata management
- Compression and optimization
- Custom processor extensions

### 3. AstralDebugOverlay

A comprehensive in-game debug overlay.

```cpp
enum class DebugPanelType {
    Performance,
    Physics,
    Rendering,
    Memory,
    Assets,
    Console,
    Configuration,
    Testing
};

class DebugPanel {
protected:
    std::string name;
    bool visible;
    
public:
    DebugPanel(const std::string& name);
    virtual ~DebugPanel() = default;
    
    const std::string& getName() const;
    
    void setVisible(bool visible);
    bool isVisible() const;
    bool toggleVisible();
    
    virtual void update(float deltaTime) = 0;
    virtual void render() = 0;
};

class PerformancePanel : public DebugPanel {
private:
    // Performance metrics
    struct MetricHistory {
        std::vector<float> values;
        size_t maxSize;
        float min, max, avg;
    };
    
    std::unordered_map<std::string, MetricHistory> metrics;
    
public:
    PerformancePanel();
    
    void trackMetric(const std::string& name, float value);
    
    void update(float deltaTime) override;
    void render() override;
};

class PhysicsPanel : public DebugPanel {
private:
    ChunkManager* chunkManager;
    MaterialRegistry* materialRegistry;
    CellularPhysics* physics;
    
    // Physics stats
    int activeCells;
    int activeChunks;
    int updatedCells;
    
    // Visualization options
    bool showChunkBoundaries;
    bool showActiveChunks;
    bool showVelocities;
    bool showTemperature;
    
public:
    PhysicsPanel(ChunkManager* chunkManager, MaterialRegistry* materialRegistry, CellularPhysics* physics);
    
    void update(float deltaTime) override;
    void render() override;
};

class DebugOverlay {
private:
    static DebugOverlay* instance;
    
    bool visible;
    std::unordered_map<DebugPanelType, std::unique_ptr<DebugPanel>> panels;
    
    // Layout
    bool dockingEnabled;
    bool fullscreen;
    
    // Input state
    bool keyPressed;
    
    DebugOverlay(); // Private constructor for singleton
    
public:
    static DebugOverlay* getInstance();
    
    void initialize();
    void shutdown();
    
    void registerPanel(DebugPanelType type, std::unique_ptr<DebugPanel> panel);
    
    void update(float deltaTime);
    void render();
    
    bool handleKeyEvent(int key, int scancode, int action, int mods);
    
    void setVisible(bool visible);
    bool isVisible() const;
    bool toggleVisible();
    
    void setPanelVisible(DebugPanelType type, bool visible);
    bool isPanelVisible(DebugPanelType type) const;
    
    void setDockingEnabled(bool enabled);
    void setFullscreen(bool fullscreen);
};
```

Features:
- Performance metrics
- Physics visualization
- Memory usage tracking
- Asset loading monitoring
- Console output
- Configuration editing
- Test execution
- Customizable layout

### 4. AstralMemoryTracker

A memory tracking system for monitoring allocations and detecting leaks.

```cpp
struct AllocationInfo {
    void* address;
    size_t size;
    std::string typeName;
    std::string file;
    int line;
    std::string function;
    std::string stackTrace;
    std::chrono::steady_clock::time_point time;
};

class MemoryTracker {
private:
    static MemoryTracker* instance;
    
    bool tracking;
    std::unordered_map<void*, AllocationInfo> allocations;
    std::mutex trackerMutex;
    
    // Statistics
    size_t totalAllocated;
    size_t peakAllocated;
    size_t totalAllocations;
    size_t activeAllocations;
    
    // Memory leaks
    std::vector<AllocationInfo> leaks;
    
    // Category tracking
    std::unordered_map<std::string, size_t> categoryAllocations;
    
    MemoryTracker(); // Private constructor for singleton
    
public:
    static MemoryTracker* getInstance();
    
    void initialize();
    void shutdown();
    
    void startTracking();
    void stopTracking();
    bool isTracking() const;
    
    void trackAllocation(void* ptr, size_t size, const std::string& typeName, const std::string& file, int line, const std::string& function);
    void trackDeallocation(void* ptr);
    
    void setCategoryForAllocations(const std::string& category);
    
    // Statistics
    size_t getTotalAllocated() const;
    size_t getPeakAllocated() const;
    size_t getTotalAllocations() const;
    size_t getActiveAllocations() const;
    
    const std::unordered_map<std::string, size_t>& getCategoryAllocations() const;
    
    // Memory leak detection
    void checkForLeaks();
    const std::vector<AllocationInfo>& getLeaks() const;
    
    void generateReport(const std::string& filePath);
    
    void renderImGui();
};

// Helper macros for tracking
#define ASTRAL_NEW(type) new (MemoryTracker::getInstance()->trackAllocation(nullptr, sizeof(type), #type, __FILE__, __LINE__, __FUNCTION__)) type
#define ASTRAL_DELETE(ptr) (MemoryTracker::getInstance()->trackDeallocation(ptr), delete ptr)
```

Features:
- Allocation tracking
- Leak detection
- Memory usage statistics
- Category-based tracking
- Stack trace recording
- Memory usage visualization
- Leak reporting
- Custom allocator integration

## Conclusion

These development tools form a comprehensive toolkit for developers working on the Astral engine. By providing detailed insights, debugging capabilities, and automation tools, they significantly enhance the development process, reduce debugging time, and help maintain high code quality.

The tools are designed to be:

1. **Integrated**: All tools work seamlessly with the engine and each other
2. **Non-intrusive**: Tools can be enabled/disabled as needed with minimal overhead when disabled
3. **Extensible**: New tools and features can be added easily
4. **User-friendly**: Clear interfaces with both code and visual components

These tools should be developed alongside the core engine, with early versions available from the beginning of development to help identify issues early in the process.