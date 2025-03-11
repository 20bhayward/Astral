#include "astral/tools/DebugUI.h"
#include "GLFW/glfw3.h"
#include <iostream>
#include <algorithm>
#include <string>

#ifndef ASTRAL_NO_IMGUI
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#endif

namespace astral {

DebugUI::DebugUI()
    : visible(true)
    , window(nullptr)
{
}

DebugUI::~DebugUI()
{
    shutdown();
}

bool DebugUI::initialize(void* window)
{
    this->window = window;
    
    // Check window
    if (!window)
    {
        std::cerr << "Invalid window for Debug UI" << std::endl;
        return false;
    }
    
#ifndef ASTRAL_NO_IMGUI
    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    
    // Enable features
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    
    // Set up style
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
    
    // Set up platform/renderer backends
    ImGui_ImplGlfw_InitForOpenGL(static_cast<GLFWwindow*>(window), true);
    ImGui_ImplOpenGL3_Init("#version 330 core");
#else
    std::cout << "Debug UI disabled: ImGui not available" << std::endl;
#endif
    
    return true;
}

void DebugUI::shutdown()
{
    if (window)
    {
#ifndef ASTRAL_NO_IMGUI
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
#endif
        window = nullptr;
    }
}

void DebugUI::beginFrame()
{
    if (!visible || !window) return;
    
#ifndef ASTRAL_NO_IMGUI
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    setupDockspace();
    renderMainMenuBar();
#endif
}

void DebugUI::endFrame()
{
    if (!visible || !window) return;
    
#ifndef ASTRAL_NO_IMGUI
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    
    // Update and render additional platform windows
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
#endif
}

void DebugUI::setVisible(bool visible)
{
    this->visible = visible;
}

bool DebugUI::isVisible() const
{
    return visible;
}

void DebugUI::toggleVisibility()
{
    visible = !visible;
}

void DebugUI::renderPerformancePanel(const PerformanceMetrics& metrics)
{
    if (!visible || !window) return;
    
#ifndef ASTRAL_NO_IMGUI
    if (ImGui::Begin("Performance"))
    {
        // Frame timing
        ImGui::Text("Frame Time: %.3f ms", metrics.frameTime * 1000.0f);
        ImGui::Text("FPS: %.1f", metrics.fps);
        ImGui::Separator();
        
        // System timings
        ImGui::Text("Update Time: %.3f ms", metrics.updateTime * 1000.0f);
        ImGui::Text("Physics Time: %.3f ms", metrics.physicsTime * 1000.0f);
        ImGui::Text("Render Time: %.3f ms", metrics.renderTime * 1000.0f);
        ImGui::Separator();
        
        // Frame time history
        static float history[100] = {};
        static int historyOffset = 0;
        
        // Add current frame time to history
        history[historyOffset] = static_cast<float>(metrics.frameTime * 1000.0f);
        historyOffset = (historyOffset + 1) % IM_ARRAYSIZE(history);
        
        // Calculate average and range
        float average = 0.0f;
        float min = FLT_MAX;
        float max = 0.0f;
        for (float value : history)
        {
            if (value > 0.0f)
            {
                average += value;
                min = std::min(min, value);
                max = std::max(max, value);
            }
        }
        average /= 100.0f;
        
        // Format overlay text
        char overlay[32];
        sprintf(overlay, "Avg %.3f ms, Min %.3f, Max %.3f", average, min == FLT_MAX ? 0.0f : min, max);
        
        // Plot frame time history
        ImGui::PlotLines("Frame Time", history, IM_ARRAYSIZE(history), historyOffset, 
                         overlay, 0.0f, max * 1.5f, ImVec2(0, 80));
        
        // CPU/GPU usage
        ImGui::Separator();
        ImGui::Text("CPU Usage: N/A");  // Would require platform-specific code
        ImGui::Text("GPU Usage: N/A");  // Would require GPU profiling integration
    }
    ImGui::End();
#else
    // Log metrics to console as fallback
    std::cout << "Performance: " 
              << "FPS: " << metrics.fps 
              << ", Frame Time: " << metrics.frameTime * 1000.0f << "ms" 
              << ", Physics: " << metrics.physicsTime * 1000.0f << "ms" 
              << ", Render: " << metrics.renderTime * 1000.0f << "ms" 
              << std::endl;
#endif
}

void DebugUI::renderPhysicsPanel(int activeChunks, int totalChunks, int activeCells, int updatedCells)
{
    if (!visible || !window) return;
    
#ifndef ASTRAL_NO_IMGUI
    if (ImGui::Begin("Physics Debug"))
    {
        // Chunk statistics
        ImGui::Text("Active Chunks: %d / %d (%.1f%%)", 
                    activeChunks, totalChunks, 
                    totalChunks > 0 ? 100.0f * activeChunks / totalChunks : 0.0f);
        
        // Cell statistics
        ImGui::Text("Active Cells: %d", activeCells);
        ImGui::Text("Updated Cells: %d", updatedCells);
        ImGui::Separator();
        
        // Physics settings
        static float gravity = 9.8f;
        if (ImGui::SliderFloat("Gravity", &gravity, 0.0f, 20.0f))
        {
            // Apply gravity change
        }
        
        static int iterationsPerFrame = 1;
        if (ImGui::SliderInt("Iterations Per Frame", &iterationsPerFrame, 1, 10))
        {
            // Apply iterations change
        }
        
        // Physics debug visualization options
        static bool showActiveChunks = true;
        ImGui::Checkbox("Show Active Chunks", &showActiveChunks);
        
        static bool showVelocity = false;
        ImGui::Checkbox("Show Velocity Vectors", &showVelocity);
        
        static bool showCollisions = false;
        ImGui::Checkbox("Show Collisions", &showCollisions);
    }
    ImGui::End();
#else
    // Log physics info to console as fallback
    std::cout << "Physics: "
              << "Chunks: " << activeChunks << "/" << totalChunks 
              << " (" << (totalChunks > 0 ? 100.0f * activeChunks / totalChunks : 0.0f) << "%)"
              << ", Cells: Active=" << activeCells << ", Updated=" << updatedCells
              << std::endl;
#endif
}

void DebugUI::renderMemoryPanel(size_t memoryUsage, int allocations)
{
    if (!visible || !window) return;
    
#ifndef ASTRAL_NO_IMGUI
    if (ImGui::Begin("Memory Usage"))
    {
        // General memory statistics
        ImGui::Text("Total Memory Usage: %.2f MB", memoryUsage / (1024.0f * 1024.0f));
        ImGui::Text("Allocations: %d", allocations);
        ImGui::Separator();
        
        // Memory breakdown (example)
        struct MemoryCategory {
            const char* name;
            size_t size;
            ImVec4 color;
        };
        
        MemoryCategory categories[] = {
            { "Physics", memoryUsage * 0.4f, ImVec4(0.2f, 0.8f, 0.2f, 1.0f) },
            { "Rendering", memoryUsage * 0.3f, ImVec4(0.8f, 0.2f, 0.2f, 1.0f) },
            { "Resources", memoryUsage * 0.2f, ImVec4(0.2f, 0.2f, 0.8f, 1.0f) },
            { "Other", memoryUsage * 0.1f, ImVec4(0.8f, 0.8f, 0.2f, 1.0f) }
        };
        
        // Display categories
        for (const auto& category : categories)
        {
            ImGui::ColorButton("##color", category.color, ImGuiColorEditFlags_NoTooltip, ImVec2(16, 16));
            ImGui::SameLine();
            ImGui::Text("%s: %.2f MB (%.1f%%)", 
                       category.name, 
                       category.size / (1024.0f * 1024.0f),
                       100.0f * category.size / memoryUsage);
        }
        
        // Memory history plot placeholder
        static float memHistory[100] = {};
        static int memHistoryOffset = 0;
        
        // Add current memory usage to history
        memHistory[memHistoryOffset] = static_cast<float>(memoryUsage / (1024.0f * 1024.0f));
        memHistoryOffset = (memHistoryOffset + 1) % IM_ARRAYSIZE(memHistory);
        
        // Plot memory history
        ImGui::Separator();
        ImGui::PlotLines("Memory Usage (MB)", memHistory, IM_ARRAYSIZE(memHistory), memHistoryOffset, 
                         nullptr, 0.0f, 0.0f, ImVec2(0, 80));
    }
    ImGui::End();
#else
    // Log memory info to console as fallback
    std::cout << "Memory: " 
              << "Total=" << (memoryUsage / (1024.0f * 1024.0f)) << " MB" 
              << ", Allocations=" << allocations 
              << std::endl;
#endif
}

void DebugUI::renderCustomPanel(const std::string& title, std::function<void()> contentFunc)
{
    if (!visible || !window) return;
    
#ifndef ASTRAL_NO_IMGUI
    if (ImGui::Begin(title.c_str()))
    {
        if (contentFunc)
        {
            contentFunc();
        }
    }
    ImGui::End();
#else
    // Custom panels not supported in fallback mode
    if (contentFunc)
    {
        std::cout << "Custom panel '" << title << "' not displayed (ImGui disabled)" << std::endl;
    }
#endif
}

void DebugUI::renderPlot(const std::string& label, const std::vector<float>& values,
                        const std::string& overlay, float scaleMin, float scaleMax,
                        const float size[2])
{
    if (!visible || !window || values.empty()) return;
    
#ifndef ASTRAL_NO_IMGUI
    ImVec2 plotSize;
    if (size)
    {
        plotSize = ImVec2(size[0], size[1]);
    }
    else
    {
        plotSize = ImVec2(0, 80);
    }
    
    ImGui::PlotLines(label.c_str(), values.data(), static_cast<int>(values.size()), 0,
                     overlay.empty() ? nullptr : overlay.c_str(), scaleMin, scaleMax, plotSize);
#else
    // Plots not supported in fallback mode
    std::cout << "Plot '" << label << "' not displayed (ImGui disabled)" << std::endl;
#endif
}

void DebugUI::setupDockspace()
{
#ifndef ASTRAL_NO_IMGUI
    static bool opt_fullscreen = true;
    static bool opt_padding = false;
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
    
    // We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
    // because it would be confusing to have two docking targets within each others.
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    if (opt_fullscreen)
    {
        const ImGuiViewport* viewport = ImGui::GetMainViewport();
        ImGui::SetNextWindowPos(viewport->WorkPos);
        ImGui::SetNextWindowSize(viewport->WorkSize);
        ImGui::SetNextWindowViewport(viewport->ID);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
        window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
        window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    }
    else
    {
        dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
    }
    
    // When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
    // and handle the pass-thru hole, so we ask Begin() to not render a background.
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;
    
    // Important: note that we proceed even if Begin() returns false (aka window is collapsed).
    // This is because we want to keep our DockSpace() active. If a DockSpace() is inactive,
    // all active windows docked into it will lose their parent and become undocked.
    // We cannot preserve the docking relationship between an active window and an inactive docking, otherwise
    // any change of dockspace/settings would lead to windows being stuck in limbo and never being visible.
    if (!opt_padding)
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", nullptr, window_flags);
    if (!opt_padding)
        ImGui::PopStyleVar();
    
    if (opt_fullscreen)
        ImGui::PopStyleVar(2);
    
    // Submit the DockSpace
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
    {
        ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
    }
    
    ImGui::End();
#endif
}

void DebugUI::renderMainMenuBar()
{
#ifndef ASTRAL_NO_IMGUI
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Save Performance Data"))
            {
                // Save performance data to file
                Profiler::getInstance().saveToFile("performance_data.json");
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Exit"))
            {
                // Request application exit
                glfwSetWindowShouldClose(static_cast<GLFWwindow*>(window), true);
            }
            
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("View"))
        {
            if (ImGui::MenuItem("Performance"))
            {
                // Show performance window
            }
            
            if (ImGui::MenuItem("Physics Debug"))
            {
                // Show physics debug window
            }
            
            if (ImGui::MenuItem("Memory Usage"))
            {
                // Show memory usage window
            }
            
            ImGui::EndMenu();
        }
        
        if (ImGui::BeginMenu("Debug"))
        {
            static bool doShowGrid = true;
            static bool doShowChunks = true;
            static bool doShowActiveCells = false;
            static bool doShowFPS = true;
            
            ImGui::MenuItem("Show Grid", nullptr, &doShowGrid);
            ImGui::MenuItem("Show Chunks", nullptr, &doShowChunks);
            ImGui::MenuItem("Show Active Cells", nullptr, &doShowActiveCells);
            ImGui::MenuItem("Show FPS", nullptr, &doShowFPS);
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Reset Simulation"))
            {
                // Reset the simulation
            }
            
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }
#endif
}

} // namespace astral