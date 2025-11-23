#include "MainWindow.h"
#include "../Core/Utility/modelloader.h"
#include "../Soundsystem/resourcemanager.h"
#include "Editor/ui_MainWindow.h"
#include "../Core/Renderer.h"
#include "../Core/Utility/BblHub.h"
#include "../Game/GameWorld.h"

#include <QFileDialog>
#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QSplitter>
#include <QStandardPaths>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QFormLayout>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QInputDialog>
#include <QMessageBox>
#include <QGroupBox>
#include <QFormLayout>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QApplication>
#include <QDir>
#include <QDateTime>
#include <QMessageBox>
#include <QFileDialog>
#include <QCoreApplication>


// Static logger widget
QPointer<QPlainTextEdit> MainWindow::messageLogWidget = nullptr;

//=============================================================================
// Constructor / Destructor
//=============================================================================

MainWindow::MainWindow(ResourceManager* resourceManager, QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , resourceManager(resourceManager)
{
    ui->setupUi(this);
    resize(1300, 850);
    setWindowTitle("BBL Engine");

    // Register global references
    BBLHub::Instance().SetMainWindow(this);
    BBLHub::Instance().SetResourceManager(resourceManager);

    // Initialize UI components
    initRenderer();
    initUI();
    initLogger();

    // Status bar message
    statusBar()->showMessage("Don't be afraid of slow progress, be afraid of standing still.");

    // Background music
    //resourceManager->toggleBackgroundMusic();
}

MainWindow::~MainWindow()
{
    delete mVulkanWindow;
    delete ui;
}

//=============================================================================
// Initialization Helpers
//=============================================================================

void MainWindow::initRenderer()
{
    mVulkanWindow = new Renderer();
    mVulkanWindow->setTitle("Renderer");
    mVulkanWindow->setWidth(1100);
    mVulkanWindow->setHeight(700);
    mVulkanWindow->initVulkan();
}

void MainWindow::initUI()
{
    // --- Vulkan widget container ---
    QWidget* vulkanWidget = QWidget::createWindowContainer(mVulkanWindow, this);
    vulkanWidget->setMinimumSize(1100, 700);
    vulkanWidget->setFocusPolicy(Qt::NoFocus);

    // --- Top bar with Play button ---
    QWidget* topBar = createTopBar();

    // --- Splitter with top bar, Vulkan, and tabs ---
    QSplitter* splitter = new QSplitter(Qt::Vertical, this);
    splitter->setChildrenCollapsible(false);
    splitter->addWidget(topBar);
    splitter->addWidget(vulkanWidget);
    splitter->addWidget(createTabWidget());

    // --- Right side panel ---
    QWidget* rightPanel = createRightPanel();

    // --- Main layout ---
    QHBoxLayout* mainLayout = new QHBoxLayout;
    mainLayout->addWidget(splitter, 4);
    mainLayout->addWidget(rightPanel, 1);

    QWidget* central = new QWidget(this);
    central->setLayout(mainLayout);
    setCentralWidget(central);

    // Set initial focus
    setFocus();
}

void MainWindow::initLogger()
{
    qInstallMessageHandler(MainWindow::messageHandler);
}

//=============================================================================
// UI Creation Helpers
//=============================================================================

QWidget* MainWindow::createTopBar()
{
    QWidget* topBar = new QWidget(this);
    QHBoxLayout* topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(0, 20, 0, 20);
    topLayout->setAlignment(Qt::AlignCenter);

    // ------Spawn buttons-----
    QPushButton* button1 = new QPushButton("Button 1", topBar);
    button1->setFixedSize(80, 40);
    button1->setStyleSheet(
        "QPushButton { background-color: #007ACC; color: white; border-radius: 6px; }"
        "QPushButton:hover { background-color: #3399FF; }"
        );

    QPushButton* button2 = new QPushButton("Button 2", topBar);
    button2->setFixedSize(80, 40);
    button2->setStyleSheet(
        "QPushButton { background-color: #007ACC; color: white; border-radius: 6px; }"
        "QPushButton:hover { background-color: #3399FF; }"
        );

    // -----Play button------
    playButton = new QPushButton("▶ Play", topBar);
    playButton->setFixedSize(100, 40);
    playButton->setStyleSheet(playButtonStyle(false));
    connect(playButton, &QPushButton::clicked, this, &MainWindow::onPlayToggled);

    // ----Button layout-----
    topLayout->addWidget(button1);
    topLayout->addWidget(button2);
    topLayout->addStretch();
    topLayout->addWidget(playButton);
    topLayout->addStretch();

    // ----Connect buttons------
    connect(button1, &QPushButton::clicked, this, &MainWindow::onButton1Clicked);
    connect(button2, &QPushButton::clicked, this, &MainWindow::onButton2Clicked);

    return topBar;
}

QTabWidget* MainWindow::createTabWidget()
{
    QTabWidget* tabWidget = new QTabWidget;
    tabWidget->setObjectName("MainTabWidget");

    // --- Logger Tab ---
    QWidget* loggerTab = new QWidget(tabWidget);
    QVBoxLayout* loggerLayout = new QVBoxLayout(loggerTab);
    messageLogWidget = new QPlainTextEdit(loggerTab);
    messageLogWidget->setReadOnly(true);
    loggerLayout->setContentsMargins(4, 4, 4, 4);
    loggerLayout->addWidget(messageLogWidget);
    loggerTab->setLayout(loggerLayout);
    tabWidget->addTab(loggerTab, tr("Logger"));

    // --- Second Tab (empty) ---
    QWidget* secondTab = new QWidget(tabWidget);
    tabWidget->addTab(secondTab, tr("Second Tab"));

    return tabWidget;
}

QWidget* MainWindow::createRightPanel()
{
    QWidget* rightPanel = new QWidget(this);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);

    // --- Scene Objects List ---
    QGroupBox* objectsGroup = new QGroupBox("Scene Objects", rightPanel);
    QVBoxLayout* objectsLayout = new QVBoxLayout(objectsGroup);
    sceneObjectList = new QListWidget(objectsGroup);
    objectsLayout->addWidget(sceneObjectList);
    objectsGroup->setLayout(objectsLayout);

    // --- Components List ---
    QGroupBox* componentsGroup = new QGroupBox("Components", rightPanel);
    QVBoxLayout* componentsLayout = new QVBoxLayout(componentsGroup);

    // Add Component button
    addComponentButton = new QPushButton("+ Add Component", componentsGroup);
    addComponentButton->setEnabled(false); // Disabled until entity is selected
    addComponentButton->setStyleSheet(
        "QPushButton { background-color: #28a745; color: white; font-weight: bold; "
        "border-radius: 4px; padding: 6px 12px; }"
        "QPushButton:hover { background-color: #34ce57; }"
        "QPushButton:pressed { background-color: #1e7e34; }"
        "QPushButton:disabled { background-color: #6c757d; }"
        );
    connect(addComponentButton, &QPushButton::clicked, this, &MainWindow::showAddComponentDialog);

    componentsLayout->addWidget(addComponentButton);

    QScrollArea* scrollArea = new QScrollArea(componentsGroup);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    componentPanelWidget = new QWidget(scrollArea);
    componentLayout = new QVBoxLayout(componentPanelWidget);
    componentLayout->setAlignment(Qt::AlignTop);
    scrollArea->setWidget(componentPanelWidget);

    componentsLayout->addWidget(scrollArea);
    componentsGroup->setLayout(componentsLayout);

    // Add both groups to the main right layout
    rightLayout->addWidget(objectsGroup, 2);
    rightLayout->addWidget(componentsGroup, 1);
    rightLayout->setContentsMargins(5, 5, 5, 5);

    // ----- Connect when clicked ------
    connect(sceneObjectList, &QListWidget::itemClicked, this, &MainWindow::onSceneObjectSelected);

    return rightPanel;
}

//=============================================================================
// UI Helpers
//=============================================================================

QString MainWindow::playButtonStyle(bool playing) const
{
    return playing ?
               "QPushButton { background-color: #b22222; color: white; font-weight: bold; border-radius: 6px; }"
               "QPushButton:hover { background-color: #cc3333; }"
               "QPushButton:pressed { background-color: #8b0000; }" :
               "QPushButton { background-color: #00FF00; color: white; font-weight: bold; border-radius: 6px; }"
               "QPushButton:hover { background-color: #505050; }"
               "QPushButton:pressed { background-color: #2a2a2a; }";
}

//=============================================================================
// Slots & Event Handlers
//=============================================================================

void MainWindow::start()
{
    qDebug("Start is called");
    mVulkanWindow->requestUpdate();
    updateSceneObjectList();
}

void MainWindow::on_action_Quit_triggered()
{
    QMessageBox::StandardButton reply = QMessageBox::question(
        this, "Quit BBL engine", "Are you sure you want to quit?",
        QMessageBox::Yes | QMessageBox::No
        );
    if (reply == QMessageBox::Yes) {
        delete mVulkanWindow;
        mVulkanWindow = nullptr;
        close();
    }
}

void MainWindow::on_action_Open_triggered()
{
    QString username = QDir::home().dirName();
    QString repeated = QString("%1! ").arg(username).repeated(280);
    QMessageBox::warning(this, "Why would you do this?", repeated, QMessageBox::No);
}

void MainWindow::onButton1Clicked()
{
    bbl::EntityID entityID = mVulkanWindow->spawnModel(
        "../../Assets/Models/Ball2.obj",
        "../../Assets/Textures/Blue.jpg",
        glm::vec3(370.0f, 200.0f, -290.0f)
        );

    bbl::EntityManager* entityManager = mVulkanWindow->getEntityManager();
    bbl::SceneManager* sceneManager = mVulkanWindow->getSceneManager();
    bbl::GameWorld* gameWorld = mVulkanWindow->getGameWorld();

    if (entityManager && entityID != bbl::INVALID_ENTITY) {
        entityManager->addComponent(entityID, bbl::Physics{});
        entityManager->addComponent(entityID, bbl::Collision{});
        entityManager->addComponent(entityID, bbl::Audio{});

        // Legger til at ballen blir tracket
        if (gameWorld && gameWorld->getTrackingSystem())
        {
            gameWorld->getTrackingSystem()->enableTracking(
                entityID,
                0.05f, // Sampler hvert 50ms
                glm::vec3(1.0f, 0.0f, 0.0f) // Setter fargen på tracen, fungerer ikke helt optimalt
                );
        }

        if (sceneManager)
        {
            sceneManager->setEntityName(entityID, "ball");
            sceneManager->markSceneDirty();
        }

        qInfo() << "Spawned ball with EntityID:" << entityID;
    }

    mVulkanWindow->recreateSwapChain();
    mVulkanWindow->requestUpdate();
    updateSceneObjectList();
}

void MainWindow::onButton2Clicked()
{
    mVulkanWindow->spawnTerrain();
    mVulkanWindow->requestUpdate();
    updateSceneObjectList();
}

void MainWindow::onSceneObjectSelected(QListWidgetItem* item)
{
    if (!mVulkanWindow) {
        qWarning() << "VulkanWindow is null, cannot select entity";
        if (addComponentButton) addComponentButton->setEnabled(false);
        return;
    }

    bbl::EntityID selectedEntityID = static_cast<bbl::EntityID>(item->data(Qt::UserRole).toULongLong());
    mVulkanWindow->setSelectedEntity(selectedEntityID);

    // Enable the Add Component button since an entity is selected
    if (addComponentButton) addComponentButton->setEnabled(true);

    // Clear and update component list
    QLayoutItem* layoutItem;
    while ((layoutItem = componentLayout->takeAt(0)) != nullptr) {
        delete layoutItem->widget();
        delete layoutItem;
    }

    // Get entity manager to access components
    auto* entityManager = mVulkanWindow->getEntityManager();
    if (!entityManager) {
        qWarning() << "EntityManager is null, cannot access components";
        if (addComponentButton) addComponentButton->setEnabled(false);
        return;
    }

    int componentCount = 0;

    // Check for Transform component
    if (entityManager->hasComponent<bbl::Transform>(selectedEntityID)) {
        const auto& transform = entityManager->getComponent<bbl::Transform>(selectedEntityID);
        QVariantMap transformFields;
        transformFields["Position X"] = transform->position.x;
        transformFields["Position Y"] = transform->position.y;
        transformFields["Position Z"] = transform->position.z;
        transformFields["Rotation X"] = transform->rotation.x;
        transformFields["Rotation Y"] = transform->rotation.y;
        transformFields["Rotation Z"] = transform->rotation.z;
        transformFields["Scale X"] = transform->scale.x;
        transformFields["Scale Y"] = transform->scale.y;
        transformFields["Scale Z"] = transform->scale.z;
        addComponentUI("Transform Component", transformFields);
        componentCount++;
    }

    // Check for Render component
    if (entityManager->hasComponent<bbl::Render>(selectedEntityID)) {
        const auto& render = entityManager->getComponent<bbl::Render>(selectedEntityID);
        QVariantMap renderFields;
        renderFields["Visible"] = render->visible;
        renderFields["Use Phong"] = render->usePhong;
        renderFields["Use Points"] = render->usePoint;
        renderFields["Use Lines"] = render->useLine;
        addComponentUI("Render Component", renderFields);
        componentCount++;
    }

    // Check for Mesh component
    if (entityManager->hasComponent<bbl::Mesh>(selectedEntityID)) {
        QVariantMap meshFields;
        meshFields["Mesh Resource"] = "MeshID";
        addComponentUI("Mesh Component", meshFields);
        componentCount++;
    }

    // Check for Texture component
    if (entityManager->hasComponent<bbl::Texture>(selectedEntityID)) {
        QVariantMap textureFields;
        textureFields["Texture Resource"] = "TextureID";
        addComponentUI("Texture Component", textureFields);
        componentCount++;
    }

    // Check for Physics component
    if (entityManager->hasComponent<bbl::Physics>(selectedEntityID)) {
        const auto& physics = entityManager->getComponent<bbl::Physics>(selectedEntityID);
        QVariantMap physicsFields;
        physicsFields["Velocity X"] = physics->velocity.x;
        physicsFields["Velocity Y"] = physics->velocity.y;
        physicsFields["Velocity Z"] = physics->velocity.z;
        physicsFields["Acceleration X"] = physics->acceleration.x;
        physicsFields["Acceleration Y"] = physics->acceleration.y;
        physicsFields["Acceleration Z"] = physics->acceleration.z;
        physicsFields["Mass"] = physics->mass;
        physicsFields["Use Gravity"] = physics->useGravity;
        addComponentUI("Physics Component", physicsFields);
        componentCount++;
    }

    // Check for Audio component
    if (entityManager->hasComponent<bbl::Audio>(selectedEntityID)) {
        const auto& audio = entityManager->getComponent<bbl::Audio>(selectedEntityID);
        QVariantMap audioFields;
        audioFields["Volume"] = audio->volume;
        audioFields["Muted"] = audio->muted;
        audioFields["Looping"] = audio->looping;
        audioFields["Attack Sound"] = QString::fromStdString(audio->attackSound);
        audioFields["Death Sound"] = QString::fromStdString(audio->deathSound);
        addComponentUI("Audio Component", audioFields);
        componentCount++;
    }

    // Check for Collision component
    if (entityManager->hasComponent<bbl::Collision>(selectedEntityID)) {
        const auto& collision = entityManager->getComponent<bbl::Collision>(selectedEntityID);
        QVariantMap collisionFields;
        collisionFields["Size X"] = static_cast<float>(collision->colliderSize.x);
        collisionFields["Size Y"] = static_cast<float>(collision->colliderSize.y);
        collisionFields["Size Z"] = static_cast<float>(collision->colliderSize.z);
        collisionFields["Is Grounded"] = collision->isGrounded;
        collisionFields["Is Colliding"] = collision->isColliding;
        collisionFields["Is Trigger"] = collision->isTrigger;
        collisionFields["Is Static"] = collision->isStatic;
        addComponentUI("Collision Component", collisionFields);
        componentCount++;
    }

    // Get entity name for logging
    const auto& entityNames = mVulkanWindow->getEntityNames();
    auto nameIt = entityNames.find(selectedEntityID);
    QString entityName = (nameIt != entityNames.end()) ?
                             QString::fromStdString(nameIt->second) :
                             QString("Unknown Entity");

    qInfo() << "Selected entity:" << entityName
            << "(ID:" << selectedEntityID << ")"
            << "with" << componentCount << "components.";

    mVulkanWindow->requestUpdate();
}

//=============================================================================
// Component Management
//=============================================================================

void MainWindow::showAddComponentDialog()
{
    auto selected = mVulkanWindow->getSelectedEntity();
    if (!selected.has_value()) {
        QMessageBox::warning(this, "No Selection", "Please select an entity first.");
        return;
    }

    bbl::EntityID entityID = selected.value();
    auto* entityManager = mVulkanWindow->getEntityManager();
    if (!entityManager) {
        qWarning() << "EntityManager is null, cannot check components";
        return;
    }

    // Create list of available components (only show components that aren't already on the entity)
    QStringList availableComponents;

    if (!entityManager->hasComponent<bbl::Physics>(entityID))
    {
        availableComponents << "Physics Component";
    }
    if (!entityManager->hasComponent<bbl::Collision>(entityID))
    {
        availableComponents << "Collision Component";
    }
    if (!entityManager->hasComponent<bbl::Audio>(entityID))
    {
        availableComponents << "Audio Component";
    }
    if (!entityManager->hasComponent<bbl::Render>(entityID))
    {
        availableComponents << "Render Component";
    }
    if (!entityManager->hasComponent<bbl::Texture>(entityID))
    {
        availableComponents << "Texture Component";
    }
    if (!entityManager->hasComponent<bbl::Mesh>(entityID))
    {
        availableComponents << "Mesh Component";
    }

    if (availableComponents.isEmpty()) {
        QMessageBox::information(this, "No Components Available",
                                 "This entity already has all available components.");
        return;
    }

    // Create selection dialog
    bool ok;
    QString selectedComponent = QInputDialog::getItem(
        this,
        "Add Component",
        "Select component to add:",
        availableComponents,
        0,
        false,
        &ok
        );

    if (ok && !selectedComponent.isEmpty()) {
        addComponentToEntity(selectedComponent);
    }
}

void MainWindow::addComponentToEntity(const QString& componentName)
{
    auto selected = mVulkanWindow->getSelectedEntity();
    if (!selected.has_value()) {
        QMessageBox::warning(this, "No Selection", "Please select an entity first.");
        return;
    }

    bbl::EntityID entityID = selected.value();
    auto* entityManager = mVulkanWindow->getEntityManager();
    if (!entityManager) {
        qWarning() << "EntityManager is null, cannot add component";
        return;
    }

    bool added = false;
    QString message;

    if (componentName == "Physics Component") {
        if (!entityManager->hasComponent<bbl::Physics>(entityID)) {
            bbl::Physics newPhysics{};
            // Set default values
            newPhysics.velocity = glm::vec3(0.0f);
            newPhysics.acceleration = glm::vec3(0.0f);
            newPhysics.mass = 1.0f;
            newPhysics.useGravity = true;

            entityManager->addComponent(entityID, newPhysics);
            added = true;
            message = "Added Physics component to entity";
        } else {
            message = "Entity already has Physics component";
        }
    }
    else if (componentName == "Collision Component") {
        if (!entityManager->hasComponent<bbl::Collision>(entityID)) {
            bbl::Collision newCollision{};
            // Set default values
            newCollision.colliderSize = glm::vec3(1.0f);
            newCollision.isGrounded = false;
            newCollision.isTrigger = false;
            newCollision.isStatic = true;

            entityManager->addComponent(entityID, newCollision);
            added = true;
            message = "Added Collision component to entity";
        } else {
            message = "Entity already has Collision component";
        }
    }
    else if (componentName == "Audio Component") {
        if (!entityManager->hasComponent<bbl::Audio>(entityID)) {
            bbl::Audio newAudio{};
            // Set default values
            newAudio.volume = 1.0f;
            newAudio.muted = false;
            newAudio.looping = false;
            newAudio.attackSound = "";
            newAudio.deathSound = "";
            newAudio.attackBuffer = 0;
            newAudio.deathBuffer = 0;
            newAudio.attackSource = 0;
            newAudio.deathSource = 0;

            entityManager->addComponent(entityID, newAudio);
            added = true;
            message = "Added Audio component to entity";
        } else {
            message = "Entity already has Audio component";
        }
    }
    else if (componentName == "Render Component") {
        if (!entityManager->hasComponent<bbl::Render>(entityID)) {
            // Check if entity has a mesh component to get mesh resource ID
            uint32_t meshResourceID = 0;
            if (auto* meshComp = entityManager->getComponent<bbl::Mesh>(entityID)) {
                meshResourceID = meshComp->meshResourceID;
            }

            bbl::Render newRender{};
            // Set default values
            newRender.meshResourceID = meshResourceID;
            newRender.textureResourceID = 0;
            newRender.visible = true;
            newRender.usePhong = false;
            newRender.usePoint = false;
            newRender.useLine = false;

            entityManager->addComponent(entityID, newRender);
            added = true;
            message = "Added Render component to entity";
        } else {
            message = "Entity already has Render component";
        }
    }
    else if (componentName == "Texture Component") {
        if (!entityManager->hasComponent<bbl::Texture>(entityID)) {
            bbl::Texture newTexture{};
            // Set default values
            newTexture.textureResourceID = 0;  // No texture initially

            entityManager->addComponent(entityID, newTexture);
            added = true;
            message = "Added Texture component to entity";
        } else {
            message = "Entity already has Texture component";
        }
    }
    else if (componentName == "Mesh Component") {
        if (!entityManager->hasComponent<bbl::Mesh>(entityID)) {
            bbl::Mesh newMesh{};
            // Set default values
            newMesh.meshResourceID = 0;  // No mesh initially

            entityManager->addComponent(entityID, newMesh);
            added = true;
            message = "Added Mesh component to entity";
        } else {
            message = "Entity already has Mesh component";
        }
    }
    else {
        QMessageBox::warning(this, "Unknown Component",
                             QString("Unknown component type: %1").arg(componentName));
        return;
    }

    // Show result message
    if (added) {
        qInfo() << message << entityID;

        // Refresh the component list by re-selecting the same entity
        QListWidgetItem* currentItem = nullptr;
        for (int i = 0; i < sceneObjectList->count(); ++i) {
            QListWidgetItem* item = sceneObjectList->item(i);
            if (item && item->data(Qt::UserRole).toULongLong() == static_cast<qulonglong>(entityID)) {
                currentItem = item;
                break;
            }
        }

        if (currentItem) {
            onSceneObjectSelected(currentItem);
        }

        // Mark scene as dirty if using scene manager
        if (auto* sceneManager = mVulkanWindow->getSceneManager()) {
            sceneManager->markSceneDirty();
        }

        mVulkanWindow->requestUpdate();
    } else {
        QMessageBox::information(this, "Component Exists", message);
    }
}

void MainWindow::removeComponentFromEntity(const QString& componentName)
{
    auto selected = mVulkanWindow->getSelectedEntity();
    if (!selected.has_value()) {
        qWarning() << "No entity selected for component removal";
        return;
    }

    bbl::EntityID entityID = selected.value();
    auto* entityManager = mVulkanWindow->getEntityManager();
    if (!entityManager) {
        qWarning() << "EntityManager is null, cannot remove component";
        return;
    }

    // Show confirmation dialog
    QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        "Remove Component",
        QString("Are you sure you want to remove the %1?").arg(componentName),
        QMessageBox::Yes | QMessageBox::No
        );

    if (reply != QMessageBox::Yes) {
        return;
    }

    bool removed = false;

    // Remove the appropriate component based on name
    if (componentName == "Physics Component") {
        if (entityManager->hasComponent<bbl::Physics>(entityID)) {
            entityManager->removeComponent<bbl::Physics>(entityID);
            removed = true;
            qInfo() << "Removed Physics component from entity" << entityID;
        }
    }
    else if (componentName == "Collision Component") {
        if (entityManager->hasComponent<bbl::Collision>(entityID)) {
            entityManager->removeComponent<bbl::Collision>(entityID);
            removed = true;
            qInfo() << "Removed Collision component from entity" << entityID;
        }
    }
    else if (componentName == "Audio Component") {
        if (entityManager->hasComponent<bbl::Audio>(entityID)) {
            entityManager->removeComponent<bbl::Audio>(entityID);
            removed = true;
            qInfo() << "Removed Audio component from entity" << entityID;
        }
    }
    else if (componentName == "Render Component") {
        if (entityManager->hasComponent<bbl::Render>(entityID)) {
            entityManager->removeComponent<bbl::Render>(entityID);
            removed = true;
            qInfo() << "Removed Render component from entity" << entityID;
        }
    }
    else if (componentName == "Texture Component") {
        if (entityManager->hasComponent<bbl::Texture>(entityID)) {
            entityManager->removeComponent<bbl::Texture>(entityID);
            removed = true;
            qInfo() << "Removed Texture component from entity" << entityID;
        }
    }
    else if (componentName == "Mesh Component") {
        if (entityManager->hasComponent<bbl::Mesh>(entityID)) {
            entityManager->removeComponent<bbl::Mesh>(entityID);
            removed = true;
            qInfo() << "Removed Mesh component from entity" << entityID;
        }
    }
    else {
        qWarning() << "Unknown component type for removal:" << componentName;
        return;
    }

    if (removed) {
        // Refresh the component list by re-selecting the same entity
        QListWidgetItem* currentItem = nullptr;
        for (int i = 0; i < sceneObjectList->count(); ++i) {
            QListWidgetItem* item = sceneObjectList->item(i);
            if (item && item->data(Qt::UserRole).toULongLong() == static_cast<qulonglong>(entityID)) {
                currentItem = item;
                break;
            }
        }

        if (currentItem) {
            onSceneObjectSelected(currentItem);
        }

        // Mark scene as dirty if using scene manager
        if (auto* sceneManager = mVulkanWindow->getSceneManager()) {
            sceneManager->markSceneDirty();
        }

        mVulkanWindow->requestUpdate();
    } else {
        qWarning() << "Component" << componentName << "was not found on entity" << entityID;
    }
}

void MainWindow::addComponentUI(const QString& name, const QVariantMap& fields)
{
    QGroupBox* group = new QGroupBox(name);
    group->setCheckable(true);
    group->setChecked(true);
    group->setStyleSheet(
        "QGroupBox { font-weight: bold; border: 1px solid #505050; border-radius: 6px; margin-top: 6px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 3px 0 3px; }"
        );

    // Create header with remove button (only for removable components)
    QWidget* headerWidget = new QWidget();
    QHBoxLayout* headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(0, 0, 0, 0);

    // Add remove button for components that can be removed (not Transform)
    if (name != "Transform Component") {
        QPushButton* removeButton = new QPushButton("×");
        removeButton->setFixedSize(20, 20);
        removeButton->setStyleSheet(
            "QPushButton { background-color: #cc3333; color: white; border-radius: 10px; font-weight: bold; }"
            "QPushButton:hover { background-color: #ff4444; }"
            "QPushButton:pressed { background-color: #aa2222; }"
            );
        removeButton->setToolTip("Remove Component");

        // Connect remove button to removal function
        connect(removeButton, &QPushButton::clicked, this, [=]() {
            removeComponentFromEntity(name);
        });

        headerLayout->addStretch();
        headerLayout->addWidget(removeButton);
    }

    QWidget* innerWidget = new QWidget();
    QFormLayout* formLayout = new QFormLayout(innerWidget);

    for (auto it = fields.begin(); it != fields.end(); it++) {
        QWidget* editor = nullptr;
        int typeId = it.value().metaType().id();

        switch (typeId) {
        case QMetaType::Bool: {
            auto* checkBox = new QCheckBox();
            checkBox->setChecked(it.value().toBool());
            editor = checkBox;

            // Live update for physics
            if (name.contains("Physics")) {
                QString field = it.key();
                connect(checkBox, &QCheckBox::toggled, this, [=](bool val) {
                    auto selected = mVulkanWindow->getSelectedEntity();
                    if (!selected.has_value()) return;
                    bbl::EntityID entityID = selected.value();
                    auto* em = mVulkanWindow->getEntityManager();
                    if (!em) return;
                    auto* phys = em->getComponent<bbl::Physics>(entityID);
                    if (!phys) {return;}
                    if (field == "Use Gravity") {
                        phys->useGravity = val;
                    }
                    mVulkanWindow->requestUpdate();
                });
            }

            if (name.contains("Render"))
            {
                QString field = it.key();
                connect(checkBox, &QCheckBox::toggled, this, [=](bool valid)
                        {
                            auto selected = mVulkanWindow->getSelectedEntity();
                            if (!selected.has_value()) return;
                            bbl::EntityID entityID = selected.value();
                            auto* em = mVulkanWindow->getEntityManager();
                            if (!em) return;
                            auto* render = em->getComponent<bbl::Render>(entityID);
                            if (!render) {return;}
                            if (field == "Visible")
                            {
                                render->visible = valid;
                            }
                            else if (field == "Use Phong")
                            {
                                render->usePhong = valid;
                            }
                            else if (field == "Use Points")
                            {
                                render->usePoint = valid;
                            }
                            else if (field == "Use Lines")
                            {
                                render->useLine = valid;
                            }
                            mVulkanWindow->recreateSwapChain();
                            mVulkanWindow->requestUpdate();
                        });
            }


            if (name.contains("Collision")) {
                QString field = it.key();
                connect(checkBox, &QCheckBox::toggled, this, [=](bool val) {
                    auto selected = mVulkanWindow->getSelectedEntity();
                    if (!selected.has_value()) return;
                    bbl::EntityID entityID = selected.value();
                    auto* em = mVulkanWindow->getEntityManager();
                    if (!em) return;
                    auto* collision = em->getComponent<bbl::Collision>(entityID);
                    if (!collision) return;

                    if (field == "Is Grounded") {
                        collision->isGrounded = val;
                    } else if (field == "Is Colliding") {
                        collision->isColliding = val;
                    } else if (field == "Is Trigger") {
                        collision->isTrigger = val;
                    } else if (field == "Is Static") {
                        collision->isStatic = val;
                    }
                    mVulkanWindow->requestUpdate();
                });
            }


            break;
        }

        case QMetaType::Float:
        {
            auto* spinBox = new QDoubleSpinBox();
            spinBox->setRange(-10000.0, 10000.0);
            spinBox->setSingleStep(0.05);
            spinBox->setValue(it.value().toDouble());
            spinBox->setDecimals(3);
            editor = spinBox;

            if (name.contains("Transform")) {
                QString field = it.key();
                connect(spinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [=](double val){
                    auto selected = mVulkanWindow->getSelectedEntity();
                    if (!selected.has_value()) return;
                    bbl::EntityID entityID = selected.value();
                    auto* em = mVulkanWindow->getEntityManager();
                    if (!em) return;
                    auto* transform = em->getComponent<bbl::Transform>(entityID);
                    if (!transform) return;

                    if (field.contains("Position"))
                    {
                        if (field.endsWith("X")) transform->position.x = val;
                        else if (field.endsWith("Y")) transform->position.y = val;
                        else if (field.endsWith("Z")) transform->position.z = val;
                    }

                    else if (field.contains("Rotation"))
                    {
                        if (field.endsWith("X")) transform->rotation.x = val;
                        else if (field.endsWith("Y")) transform->rotation.y = val;
                        else if (field.endsWith("Z")) transform->rotation.z = val;
                    }

                    else if (field.contains("Scale"))
                    {
                        if (field.endsWith("X")) transform->scale.x = val;
                        else if (field.endsWith("Y")) transform->scale.y = val;
                        else if (field.endsWith("Z")) transform->scale.z = val;
                    }
                    mVulkanWindow->requestUpdate();
                });
            }

            if (name.contains("Physics"))
            {
                QString field = it.key();
                connect(spinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [=](double val)
                        {
                            auto selected = mVulkanWindow->getSelectedEntity();
                            if (!selected.has_value()) {return;}
                            bbl::EntityID entityID = selected.value();
                            auto* em = mVulkanWindow->getEntityManager();
                            if (!em) {return;}
                            auto* phys = em->getComponent<bbl::Physics>(entityID);
                            if(!phys) {return;}

                            if (field.contains("Velocity"))
                            {
                                if (field.endsWith("X")) phys->velocity.x = val;
                                else if (field.endsWith("Y")) phys->velocity.y = val;
                                else if (field.endsWith("Z")) phys->velocity.z = val;
                            }

                            else if (field.contains("Acceleration"))
                            {
                                if(field.endsWith("X")) phys->acceleration.x = val;
                                else if (field.endsWith("Y")) phys->acceleration.y = val;
                                else if (field.endsWith("Z")) phys->acceleration.z = val;
                            }

                            else if(field == "Mass") {
                                phys->mass = val;
                            }
                            mVulkanWindow->requestUpdate();
                        });
            }

            if (name.contains("Collision")) {
                QString field = it.key();
                connect(spinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, [=](double val) {
                    auto selected = mVulkanWindow->getSelectedEntity();
                    if (!selected.has_value()) return;
                    bbl::EntityID entityID = selected.value();
                    auto* em = mVulkanWindow->getEntityManager();
                    if (!em) return;
                    auto* collision = em->getComponent<bbl::Collision>(entityID);
                    if (!collision) return;

                    if (field.contains("Size"))
                    {
                        if      (field.endsWith("X"))      collision->colliderSize.x = val;
                        else if (field.endsWith("Y"))      collision->colliderSize.y = val;
                        else if (field.endsWith("Z"))      collision->colliderSize.z = val;
                    }
                    mVulkanWindow->requestUpdate();
                });
            }
            break;
        }

        default: {
            auto* lineEdit = new QLineEdit(it.value().toString());
            editor = lineEdit;

            if(name.contains("Audio")) {
                QString field = it.key();
                connect(lineEdit, &QLineEdit::editingFinished, this, [=]() {
                    auto selected = mVulkanWindow->getSelectedEntity();
                    if(!selected.has_value()) {return;}
                    bbl::EntityID entityID = selected.value();
                    auto* em = mVulkanWindow->getEntityManager();
                    if(!em) {return;}
                    auto* audio = em->getComponent<bbl::Audio>(entityID);
                    if (!audio) {return;}

                    QString value = lineEdit->text();
                    if (field == "Attack Sound") {
                        audio->attackSound = value.toStdString();
                        audio->attackBuffer = resourceManager->loadSound(audio->attackSound);
                        audio->attackSource = resourceManager->createSource(audio->attackBuffer);
                    } else if (field == "Death Sound") {
                        audio->deathSound = value.toStdString();
                        audio->deathBuffer = resourceManager->loadSound(audio->deathSound);
                        audio->deathSource = resourceManager->createSource(audio->deathBuffer);
                    }
                    mVulkanWindow->requestUpdate();
                });
            }
            break;
        }
        }

        formLayout->addRow(it.key() + ":", editor);
    }

    QVBoxLayout* groupLayout = new QVBoxLayout();

    // Add header with remove button if it exists
    if (headerWidget->layout() && headerWidget->layout()->count() > 0) {
        groupLayout->addWidget(headerWidget);
    } else {
        delete headerWidget; // Clean up if not used
    }

    groupLayout->addWidget(innerWidget);
    group->setLayout(groupLayout);

    //Show/hide when clicked
    connect(group, &QGroupBox::toggled, innerWidget, &QWidget::setVisible);
    componentLayout->addWidget(group);
}

//=============================================================================
// Lists
//=============================================================================

void MainWindow::updateSceneObjectList()
{
    sceneObjectList->clear();
    if (!mVulkanWindow) return;

    auto* sceneManager = mVulkanWindow->getSceneManager();
    if (!sceneManager) return;

    const auto& entityNames = sceneManager->getEntityNames();
    for (const auto& [entityID, name] : entityNames) {
        QListWidgetItem* item = new QListWidgetItem(
            QString::fromStdString(name), sceneObjectList
            );
        item->setData(Qt::UserRole, static_cast<qulonglong>(entityID));
        sceneObjectList->addItem(item);
    }

    qInfo() << "Scene object list updated with" << entityNames.size() << "entities.";
}

//=============================================================================
// Logger
//=============================================================================

void MainWindow::messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    if (!messageLogWidget) return;

    QString level, color;
    switch (type) {
    case QtDebugMsg:    level = "Debug";    color = "white";    break;
    case QtInfoMsg:     level = "Info";     color = "blue";     break;
    case QtWarningMsg:  level = "Warning";  color = "orange";   break;
    case QtCriticalMsg: level = "Critical"; color = "red";      break;
    case QtFatalMsg:    level = "Fatal";    color = "darkred";  break;
    }

    QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
    QString location = (context.file && context.line > 0) ?
                           QString("%1:%2").arg(context.file).arg(context.line) : "unknown";
    QString formattedMsg = QString("<span style='color:%1'>[%2] [%3] [%4] %5</span>")
                               .arg(color, timestamp, level, location, msg.toHtmlEscaped());

    messageLogWidget->appendHtml(formattedMsg);
    if (type == QtFatalMsg) abort();
}

//=============================================================================
// Play / Stop Toggle
//=============================================================================

void MainWindow::onPlayToggled()
{
    auto* gWorld = mVulkanWindow->getGameWorld();

    isPlaying = !isPlaying;
    playButton->setText(isPlaying ? "⏹ Stop" : "▶ Play");
    playButton->setStyleSheet(playButtonStyle(isPlaying));

    if(gWorld)
    {
        gWorld->setPaused(!isPlaying);
    }

    qInfo() << (isPlaying ? "Play mode started." : "Play mode stopped.");
}

//=============================================================================
// Scene Management
//=============================================================================

void MainWindow::on_action_NewScene_triggered()
{
    if (!mVulkanWindow) return;

    auto* sceneManager = mVulkanWindow->getSceneManager();
    if (!sceneManager) return;

    if (sceneManager->hasUnsavedChanges()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Unsaved Changes", "Current scene has unsaved changes. Continue?",
            QMessageBox::Yes | QMessageBox::No
            );
        if (reply == QMessageBox::No) return;
    }

    sceneManager->createNewScene("New Scene");
    updateSceneObjectList();
    mVulkanWindow->recreateSwapChain();
}

void MainWindow::on_action_SaveScene_triggered()
{
    if (!mVulkanWindow) return;

    auto* sceneManager = mVulkanWindow->getSceneManager();
    if (!sceneManager) return;

    if (sceneManager->getCurrentSceneFilepath().empty()) {
        on_action_SaveSceneAs_triggered();
        return;
    }

    if (sceneManager->saveCurrentScene()) {
        QMessageBox::information(this, "Success", "Scene saved successfully!");
    } else {
        QMessageBox::critical(this, "Error", QString::fromStdString(sceneManager->getLastError()));
    }
}

void MainWindow::on_action_SaveSceneAs_triggered()
{
    if (!mVulkanWindow) return;

    auto* sceneManager = mVulkanWindow->getSceneManager();
    if (!sceneManager) return;

    QString scenesPath = QDir(QCoreApplication::applicationDirPath()).filePath("../../Scenes/");
    QDir().mkpath(scenesPath);

    QString filepath = QFileDialog::getSaveFileName(
        this, "Save Scene As", scenesPath + "MyScene.scene",
        "Scene Files (*.scene);;All Files (*)"
        );

    if (filepath.isEmpty()) return;

    if (sceneManager->saveCurrentScene(filepath.toStdString())) {
        QMessageBox::information(this, "Success", "Scene saved successfully!");
    } else {
        QMessageBox::critical(this, "Error", QString::fromStdString(sceneManager->getLastError()));
    }
}

void MainWindow::on_action_LoadScene_triggered()
{
    if (!mVulkanWindow) return;

    auto* sceneManager = mVulkanWindow->getSceneManager();
    if (!sceneManager) return;

    if (sceneManager->hasUnsavedChanges()) {
        QMessageBox::StandardButton reply = QMessageBox::question(
            this, "Unsaved Changes", "Current scene has unsaved changes. Continue?",
            QMessageBox::Yes | QMessageBox::No
            );
        if (reply == QMessageBox::No) return;
    }

    QString scenesPath = QDir(QCoreApplication::applicationDirPath()).filePath("../../Scenes/");
    QString filepath = QFileDialog::getOpenFileName(
        this, "Load Scene", scenesPath, "Scene Files (*.scene);;All Files (*)"
        );

    if (filepath.isEmpty()) return;

    if (sceneManager->loadScene(filepath.toStdString())) {
        QMessageBox::information(this, "Success", "Scene loaded successfully!");
        updateSceneObjectList();

        const auto& entityNames = sceneManager->getEntityNames();
        for (const auto& [id, name] : entityNames) {
            if (name == "Terrain") {
                mVulkanWindow->getGameWorld()->setTerrainEntity(id);
                qInfo() << "Terrain entity found and set for collision system";
                break;
            }
        }

        mVulkanWindow->recreateSwapChain();
    }
}


