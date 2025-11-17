#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPointer>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QListWidget>
#include <qboxlayout.h>

#include "../Game/GameWorld.h"

// Forward declarations
class ResourceManager;
class Renderer;

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(ResourceManager* resourceMgr, QWidget* parent = nullptr);
    ~MainWindow() override;

    /// Starts the main rendering or simulation loop.
    void start();

    /// Pointer to the global message log text box (used by qInstallMessageHandler)
    static QPointer<QPlainTextEdit> messageLogWidget;

private:
    //=========================================================================
    // Core Members
    //=========================================================================
    Ui::MainWindow* ui = nullptr;
    ResourceManager* resourceManager = nullptr;
    Renderer* mVulkanWindow = nullptr;

    // Game world management
    bbl::GameWorld mGameWorld;

    //=========================================================================
    // UI Components
    //=========================================================================
    QPushButton* playButton = nullptr;
    bool isPlaying = false;
    void updateSceneObjectList();
    int selectedEntityIndex = -1;
    //=========================================================================
    // Initialization Helpers
    //=========================================================================
    void initRenderer();
    void initUI();
    void initLogger();

    QWidget* createTopBar();
    QTabWidget* createTabWidget();
    QWidget* createRightPanel();
    QString playButtonStyle(bool playing) const;
    QListWidget* sceneObjectList;
    QListWidget* componentList;
    QWidget* componentPanelWidget = nullptr;
    QVBoxLayout* componentLayout = nullptr;

    //=========================================================================
    // Logger
    //=========================================================================
    static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg);

private slots:
    //=========================================================================
    // Action Slots
    //=========================================================================
    void on_action_Quit_triggered();
    void on_action_Open_triggered();

    //=========================================================================
    // UI Buttons
    //=========================================================================
    void onPlayToggled();
    void onButton1Clicked();
    void onButton2Clicked();

    //=========================================================================
    // UI Clicked
    //=========================================================================
    void onSceneObjectSelected(QListWidgetItem* item);
    void addComponentUI(const QString& name, const QVariantMap& fields);

    //For Saving and Loading Scenes
    void on_action_SaveScene_triggered();
    void on_action_SaveSceneAs_triggered();
    void on_action_LoadScene_triggered();
    void on_action_NewScene_triggered();
};

#endif // MAINWINDOW_H
