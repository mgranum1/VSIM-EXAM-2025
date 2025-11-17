#ifndef BBLHUB_H
#define BBLHUB_H

class LuaManager;
class Renderer;
class Physics;
class Camera;
class MainWindow;
class ResourceManager;

class BBLHub
{
public:
    static BBLHub& Instance()
    {
        static BBLHub instance;
        return instance;
    }


    BBLHub(const BBLHub&) = delete;
    BBLHub& operator=(const BBLHub&) = delete;

    // Accessors
    void SetMainWindow(MainWindow* mw) { mainWindow = mw; }
    void SetResourceManager(ResourceManager* rm) { resourceManager = rm; }
    void SetCamera(Camera* c) { camera = c; }
    void SetPhysics(Physics* p) { physics = p; }
    void setRenderer(Renderer* _renderer){ renderer = _renderer;}
    void setLuaManager(LuaManager* lua){luamanager = lua;}

    MainWindow* GetMainWindow() const { return mainWindow; }
    ResourceManager* GetResourceManager() const { return resourceManager; }
    Camera* GetCamera() const { return camera; }
    Physics* GetPhysics() const { return physics; }
    Renderer* getRenderer() const {return renderer; }
    LuaManager* getLuaManager() const {return luamanager;}

private:
    BBLHub() = default; // private constructor
    ~BBLHub() = default;

    MainWindow* mainWindow = nullptr;
    ResourceManager* resourceManager = nullptr;
    Camera* camera = nullptr;
    Physics* physics = nullptr;
    Renderer* renderer = nullptr;
    LuaManager * luamanager = nullptr;
};

#endif // BBLHUB_H
