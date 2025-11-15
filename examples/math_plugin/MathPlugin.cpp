#include "hotplugpp/IPlugin.hpp"
#include <iostream>
#include <cmath>
#include <vector>

/**
 * @brief A more complex plugin demonstrating state management and computations
 * 
 * This plugin calculates various mathematical sequences and demonstrates:
 * - Stateful plugin behavior
 * - More complex update logic
 * - Resource management
 */
class MathPlugin : public hotplugpp::IPlugin {
public:
    MathPlugin() 
        : m_frameCount(0)
        , m_accumulatedTime(0.0f)
        , m_fibIndex(0) {
    }

    ~MathPlugin() override = default;

    bool onLoad() override {
        std::cout << "[MathPlugin] Initializing..." << std::endl;
        
        // Initialize Fibonacci sequence
        m_fibonacci.clear();
        m_fibonacci.push_back(0);
        m_fibonacci.push_back(1);
        
        std::cout << "[MathPlugin] Ready! Computing mathematical sequences." << std::endl;
        return true;
    }

    void onUnload() override {
        std::cout << "[MathPlugin] Shutting down..." << std::endl;
        std::cout << "[MathPlugin] Statistics:" << std::endl;
        std::cout << "  Total frames: " << m_frameCount << std::endl;
        std::cout << "  Total time: " << m_accumulatedTime << " seconds" << std::endl;
        std::cout << "  Fibonacci numbers computed: " << m_fibonacci.size() << std::endl;
        
        if (!m_fibonacci.empty()) {
            std::cout << "  Last Fibonacci number: " << m_fibonacci.back() << std::endl;
        }
    }

    void onUpdate(float deltaTime) override {
        m_frameCount++;
        m_accumulatedTime += deltaTime;

        // Every 120 frames (approximately 2 seconds at 60 FPS)
        if (m_frameCount % 120 == 0) {
            computeNextFibonacci();
            
            // Calculate some interesting values
            double sinValue = std::sin(m_accumulatedTime);
            double cosValue = std::cos(m_accumulatedTime);
            
            std::cout << "[MathPlugin] Update #" << m_frameCount << std::endl;
            std::cout << "  Time: " << m_accumulatedTime << "s" << std::endl;
            std::cout << "  sin(time): " << sinValue << std::endl;
            std::cout << "  cos(time): " << cosValue << std::endl;
            
            if (m_fibonacci.size() > 0) {
                std::cout << "  Fibonacci[" << (m_fibonacci.size() - 1) << "]: " 
                          << m_fibonacci.back() << std::endl;
            }
            std::cout << std::endl;
        }
    }

    const char* getName() const override {
        return "MathPlugin";
    }

    hotplugpp::Version getVersion() const override {
        return hotplugpp::Version(1, 0, 0);
    }

    const char* getDescription() const override {
        return "Demonstrates state management with mathematical computations";
    }

private:
    void computeNextFibonacci() {
        if (m_fibonacci.size() < 2) return;
        
        uint64_t next = m_fibonacci[m_fibonacci.size() - 1] + 
                        m_fibonacci[m_fibonacci.size() - 2];
        
        // Prevent overflow by limiting sequence length
        if (next < m_fibonacci.back()) {
            std::cout << "[MathPlugin] Fibonacci sequence overflow detected, resetting..." << std::endl;
            m_fibonacci.clear();
            m_fibonacci.push_back(0);
            m_fibonacci.push_back(1);
        } else {
            m_fibonacci.push_back(next);
        }
    }

    uint64_t m_frameCount;
    float m_accumulatedTime;
    size_t m_fibIndex;
    std::vector<uint64_t> m_fibonacci;
};

// Export the plugin
HOTPLUGPP_CREATE_PLUGIN(MathPlugin)
