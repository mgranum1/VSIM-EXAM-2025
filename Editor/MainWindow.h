#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPointer>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QListWidget>
#include <qboxlayout.h>
#include <QInputDialog>
#include <QMessageBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QScrollArea>
#include <QTabWidget>
#include <QSplitter>
#include <QHBoxLayout>
#include <QVBoxLayout>
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
    QPushButton* addComponentButton = nullptr;  // Component management button
    bool isPlaying = false;
    int selectedEntityIndex = -1;

    // UI Widgets
    QListWidget* sceneObjectList = nullptr;
    QListWidget* componentList = nullptr;
    QWidget* componentPanelWidget = nullptr;
    QVBoxLayout* componentLayout = nullptr;

    // Spawne baller med tids intervall
    int ballsSpawned = 0;
    int maxBallsSpawn = 100;
    void spawnBallsDelay();


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

    //=========================================================================
    // Helper Functions
    //=========================================================================
    void updateSceneObjectList();

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
    void onButton3Clicked();
    void onButton4Clicked();
    void onButton5Clicked();

    //=========================================================================
    // UI Interactions
    //=========================================================================
    void onSceneObjectSelected(QListWidgetItem* item);
    void addComponentUI(const QString& name, const QVariantMap& fields);

    //=========================================================================
    // Component Management
    //=========================================================================
    void showAddComponentDialog();
    void addComponentToEntity(const QString& componentName);
    void removeComponentFromEntity(const QString& componentName);

    //=========================================================================
    // Scene Management
    //=========================================================================
    void on_action_SaveScene_triggered();
    void on_action_SaveSceneAs_triggered();
    void on_action_LoadScene_triggered();
    void on_action_NewScene_triggered();
};

#endif // MAINWINDOW_H
