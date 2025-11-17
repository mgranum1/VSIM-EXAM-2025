    #ifndef RENDERER_H
    #define RENDERER_H

    #include <QWindow>
    #include <vulkan/vulkan_core.h>
    #include <string>
    #include <vector>
    #include <unordered_map>
    #include "Camera.h"
    #include "../ECS/Entity/EntityManager.h"
    #include "../ECS/Entity/SceneManager.h"
    #include "../Core/Utility/Vertex.h"
    #include "../Game/GameWorld.h"



    //Forward declarations
    struct SwapChainSupportDetails;
    struct QueueFamilyIndices;

    class Renderer : public QWindow
    {
        Q_OBJECT
    public:
        explicit Renderer(QWindow* parent = nullptr);
        ~Renderer();

        void initVulkan();


    public:

        // Get all entities that can be rendered
        bbl::SceneManager* getSceneManager() { return sceneManager.get(); }
        bbl::GameWorld* getGameWorld() { return &m_gameWorld; }
        bbl::GameWorld* setGameWorld(){return &m_gameWorld;}
        std::vector<bbl::EntityID> getRenderableEntities()
        {
            return entityManager->getEntitiesWith<bbl::Transform, bbl::Render>();
        }

        // Get entities map
        const std::unordered_map<bbl::EntityID, std::string>& getEntityNames() const {
            return entityNames;
        }

        // Get entityManager
        bbl::EntityManager* getEntityManager() const {
            return entityManager.get();
        }

        // Set selected entity using EntityID
        void setSelectedEntity(bbl::EntityID entityID)
        {
            mSelectedEntityID = entityID;
        }

        // Get currently selected entity
        std::optional<bbl::EntityID> getSelectedEntity() const {
            return mSelectedEntityID;
        }

        // Get entity count for debugging
        size_t getEntityCount()
        {
            return entityManager->getEntityCount();
        }

        void keyPressEvent(QKeyEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void keyReleaseEvent(QKeyEvent* event) override;
        void spawnTerrain();

        Camera* camera;

        bbl::EntityID spawnModel(const std::string& modelPath, const std::string& texturePath, const glm::vec3& basePosition);
        void recreateSwapChain();

    protected:
        //Qt event handlers - called when requestUpdate(); is called
        void exposeEvent(QExposeEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        bool event(QEvent* event) override;

    private:

        //Editor Camera
        bool keyW = {false};
        bool keyA = {false};
        bool keyS = {false};
        bool keyD = {false};
        bool keyQ = {false};
        bool keyE = {false};
        // class GLFWwindow* window;
        //GLFWwindow* window{nullptr};
        //QWindow* window{ nullptr }; //this object IS a QWindow

        bbl::GameWorld m_gameWorld;
        std::unique_ptr<bbl::SceneManager> sceneManager;

        void createTerrainEntity(bbl::GameWorld *gameWorld);
        bool rightMouseHeld = false;
        QPoint lastMousePos;
        float deltaTime =  0.016f;

        VkInstance instance;
        VkDebugUtilsMessengerEXT debugMessenger;
        VkSurfaceKHR surface;

        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
        VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
        VkDevice device;

        std::unique_ptr<bbl::GPUResourceManager> GPUresources;
        std::unique_ptr<bbl::EntityManager> entityManager;

        // Optional: we can use it for tracking entity names (for debugging)
        std::unordered_map<bbl::EntityID, std::string> entityNames;


        VkImageView defaultTextureImageView = VK_NULL_HANDLE;
        VkSampler   defaultTextureSampler   = VK_NULL_HANDLE;



        VkQueue graphicsQueue;
        VkQueue presentQueue;

        VkSwapchainKHR swapChain;
        std::vector<VkImage> swapChainImages;
        VkFormat swapChainImageFormat;
        VkExtent2D swapChainExtent;
        std::vector<VkImageView> swapChainImageViews;
        std::vector<VkFramebuffer> swapChainFramebuffers;

        VkRenderPass renderPass;
        VkDescriptorSetLayout descriptorSetLayout;
        VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;

        VkPipeline graphicsPipeline;
        VkPipeline phongPipeline;

        VkCommandPool commandPool;

        VkImage colorImage;
        VkDeviceMemory colorImageMemory;
        VkImageView colorImageView;

        VkImage depthImage;
        VkDeviceMemory depthImageMemory;
        VkImageView depthImageView;

        uint32_t mipLevels;
        VkImage textureImage;
        VkDeviceMemory textureImageMemory;
        VkImageView textureImageView;
        VkSampler textureSampler;

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemory;
        VkBuffer indexBuffer;
        VkDeviceMemory indexBufferMemory;

        std::vector<VkBuffer> uniformBuffers;
        std::vector<VkDeviceMemory> uniformBuffersMemory;

        VkDescriptorPool descriptorPool;
        std::vector<VkDescriptorSet> descriptorSets;

        std::vector<VkCommandBuffer> commandBuffers;

        std::vector<VkSemaphore> imageAvailableSemaphores;
        std::vector<VkSemaphore> renderFinishedSemaphores;
        std::vector<VkFence> inFlightFences;
        std::vector<VkFence> imagesInFlight;
        size_t currentFrame = 0;

        bool framebufferResized = false;

        // void initWindow();

        // static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

        // ---- Functions ----
        void createEntitiesFromModel(const bbl::ModelData& modelData,
                                     const glm::vec3& basePosition, bool usePhong);


        void drawFrame();
        void cleanup();
        void cleanupSwapChain();
        void createInstance();
        void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
        void setupDebugMessenger();
        void createSurface();
        void pickPhysicalDevice();
        void createLogicalDevice();
        void createSwapChain();
        void createImageViews();
        void createRenderPass();
        void createDescriptorSetLayout();
        void createGraphicsPipeline(std::string, std::string, VkPipeline&);
        void createFramebuffers();
        void createCommandPool();
        void createColorResources();
        void createDepthResources();
        VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
        VkFormat findDepthFormat();
        bool hasStencilComponent(VkFormat format);
        void createTextureImage();
        void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels);
        VkSampleCountFlagBits getMaxUsableSampleCount();
        void createTextureImageView();
        void createTextureSampler();
        VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels);
        void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples,
                         VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties,
                         VkImage& image, VkDeviceMemory& imageMemory);
        void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels);
        void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height);
        void loadModel();
        void createVertexBuffer();
        void createIndexBuffer();
        void createUniformBuffers();
        void createDescriptorPool();
        void createDescriptorSets();
        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
        VkCommandBuffer beginSingleTimeCommands();
        void endSingleTimeCommands(VkCommandBuffer commandBuffer);
        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
        void createCommandBuffers();
        void createSyncObjects();
        void updateUniformBuffer(uint32_t currentImage);
        VkShaderModule createShaderModule(const std::vector<char>& code);
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
        bool isDeviceSuitable(VkPhysicalDevice device);
        bool checkDeviceExtensionSupport(VkPhysicalDevice device);
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
        std::vector<const char*> getRequiredExtensions();
        bool checkValidationLayerSupport();
        static std::vector<char> readFile(const std::string& filename);
        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData);
        glm::vec3 mNextSpawnOffset = glm::vec3(0.0f, 0.0f, 0.0f);

        // Changed from index-based to EntityID-based selection
        std::optional<bbl::EntityID> mSelectedEntityID;
    };



    #endif // RENDERER_H
