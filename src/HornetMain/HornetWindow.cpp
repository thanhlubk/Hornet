#include "HornetWindow.h"
#include <HornetView/GLViewWindow.h>
#include "ui_HornetWindow.h"
#include <HornetView/HViewDef.h>
#include <HornetBase/HINode.h>
#include <HornetBase/HIElement.h>
#include <HornetBase/AppBase.h>
#include <HornetBase/DocumentManager.h>
#include "DocumentModel.h"
#include <QBoxLayout>
#include <HornetBase/HILbcForce.h>
#include <HornetBase/HILbcConstraint.h>
#include <QDir>
#include "Csv.h"
#include <HornetExecute/HESolveCrackPropagation.h>
#include <HornetExecute/HESolveLinearAnalysis.h>
#include <HornetExecute/HESolveDef.h>
#include <QFileDialog>
#include <HornetBase/HIResult.h>

namespace
{ 
// ---- tiny helpers ----
}

HornetWindow::HornetWindow(AppBase* app, QWidget *parent)
    : QMainWindow(parent), ui(new Ui::HornetWindow), m_app(app)
{
    m_pViewWidget = nullptr;
    // Init window
    initWindow();
}

HornetWindow::~HornetWindow()
{
    ui->centralwidget->layout()->removeWidget(m_pViewWidget);
    m_pViewWidget->setParent(nullptr);
    m_pViewWidget = nullptr;
    delete ui;
}

void HornetWindow::initWindow()
{
    // Remove native title bar
    // setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    // setAttribute(Qt::WA_TranslucentBackground, false);

    // Setup UI
    ui->setupUi(this);

    ui->comboBoxStep->clear();
    ui->comboBoxStep->addItem("Initial", 0);
    ui->comboBoxStep->setCurrentIndex(0);

    ui->comboBoxResultType->clear();

    ui->comboBoxType->clear();
    ui->comboBoxType->addItem("Plane Strain", 0);
    ui->comboBoxType->addItem("Plane Stress", 1);
    ui->comboBoxType->setCurrentIndex(0);

    ui->comboBoxAnalysis->clear();
    ui->comboBoxAnalysis->addItem("Static", 0);
    ui->comboBoxAnalysis->addItem("Modal", 1);
    ui->comboBoxAnalysis->setCurrentIndex(0);

    ui->lineEditModulus->setText("2e11");
    ui->lineEditDensity->setText("1000");
    ui->lineEditPoison->setText("0.3");

    ui->lineEditThickness->setText("1");
    ui->lineEditGrowthLength->setText("0.005");
    ui->lineEditSifRadius->setText("0.01");
    ui->lineEditNumStep->setText("12");

    connect(ui->pushButtonImport, &QPushButton::clicked, this, &HornetWindow::onImportModel);
    connect(ui->pushButtonSolve, &QPushButton::clicked, this, &HornetWindow::onSolve);
    connect(ui->pushButtonShowResult, &QPushButton::clicked, this, &HornetWindow::onShowResult);
    connect(ui->pushButtonUnshowResult, &QPushButton::clicked, this, &HornetWindow::onUnshowResult);

    connect(ui->pushButtonToggleMeshline, &QPushButton::clicked, this, &HornetWindow::onToggleMeshLine);
    connect(ui->pushButtonToggleNode, &QPushButton::clicked, this, &HornetWindow::onToggleNode);
    connect(ui->pushButtonToggleLbc, &QPushButton::clicked, this, &HornetWindow::onToggleLbc);
    connect(ui->pushButtonToggleDeformation, &QPushButton::clicked, this, &HornetWindow::onToggleDeformation);
    connect(ui->comboBoxStep, &QComboBox::currentIndexChanged, this, &HornetWindow::onStepChanged);
    connect(ui->checkBoxCrack, &QCheckBox::toggled, this, &HornetWindow::onEnableCrack);

    onEnableCrack(ui->checkBoxCrack->isChecked());
    createDocumentModel();
}

void HornetWindow::onImportModel()
{
    auto pDoc = dynamic_cast<DocumentModel*>(m_app->docs()->activeDocument());
    if (!pDoc)
        return;
    
    auto pDb = pDoc->database();
    if (!pDb)
        return;

    QString strDatatDir = QFileDialog::getExistingDirectory(
        this,                          // Parent widget
        tr("Select Import Directory"), // Dialog title
        "/home",                       // Starting directory (optional)
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );

    // 2. Check if the user actually selected something
    if (strDatatDir.isEmpty()) {
        // User clicked "Cancel"
        return; 
    }
    
    // auto strDatatDir = QStringLiteral("C:\\Data\\Source\\Temp\\fem_solver\\bracket_tet4_finer_with_hole_over_5000_nodes");
    auto nodePath = QDir(strDatatDir).filePath(QStringLiteral("Node_file.csv"));
    auto elementPath = QDir(strDatatDir).filePath(QStringLiteral("Element_file.csv"));
    auto loadPath = QDir(strDatatDir).filePath(QStringLiteral("Load_file.csv"));
    auto bcPath = QDir(strDatatDir).filePath(QStringLiteral("BC_file.csv"));

    QString crackPath = QString("");
    m_vecCrack.clear();
    if (ui->checkBoxCrack->isChecked())
    {
        crackPath = QDir(strDatatDir).filePath(QStringLiteral("Crack_file.csv"));
        CsvTable crackCsv(crackPath.toStdString());
        for (std::size_t i = 0; i < crackCsv.rowCount(); ++i)
        {
            auto crackNum = static_cast<Id>(crackCsv.getInt(i, "Crack Number"));
            auto X = crackCsv.getDouble(i, "Point X");
            auto Y = crackCsv.getDouble(i, "Point Y");

            if (m_vecCrack.size() < crackNum)
                m_vecCrack.resize(crackNum);

            m_vecCrack[crackNum - 1].push_back({X, Y});
        }
    }

    pDb->beginTransaction();
    CsvTable nodeCsv(nodePath.toStdString());
    for (std::size_t i = 0; i < nodeCsv.rowCount(); ++i)
    {
        auto id = nodeCsv.getDouble(i, "Node Number");
        auto x = nodeCsv.getDouble(i, "X Location (m)");
        auto y = nodeCsv.getDouble(i, "Y Location (m)");

        double z = 0.0;
        if (!ui->checkBoxCrack->isChecked())
            z = nodeCsv.getDouble(i, "Z Location (m)");

        pDb->emplace<HINode>(static_cast<Id>(id));
        auto pNode = pDb->checkOut<HINode>(static_cast<Id>(id));
        pNode->setPosition({x, y, z});
    }

    CsvTable elementCsv(elementPath.toStdString());
    for (std::size_t i = 0; i < elementCsv.rowCount(); ++i)
    {
        auto id = static_cast<Id>(i + 1);
        if (elementCsv.getString(i, "Element Type") == "TET4")
        {
            std::vector<Id> nodeIds = {
                static_cast<Id>(elementCsv.getInt(i, "Nodes 1")),
                static_cast<Id>(elementCsv.getInt(i, "Nodes 2")),
                static_cast<Id>(elementCsv.getInt(i, "Nodes 3")),
                static_cast<Id>(elementCsv.getInt(i, "Nodes 4"))
            };

            pDb->emplace<HIElementTet4>(id);
            auto pElem = pDb->checkOut<HIElementTet4>(id);
            std::vector<HCursor*> nodeCursors;
            for (const auto& nodeId : nodeIds)
            {
                auto pNode = pDb->checkOut<HINode>(nodeId);
                if (pNode)
                    nodeCursors.push_back(pNode->getCursor());
            }
            pElem->setNodes(nodeCursors);
        }
        else if (elementCsv.getString(i, "Element Type") == "PRISM6")
        {
            std::vector<Id> nodeIds = {
                static_cast<Id>(elementCsv.getInt(i, "Nodes 1")),
                static_cast<Id>(elementCsv.getInt(i, "Nodes 2")),
                static_cast<Id>(elementCsv.getInt(i, "Nodes 3")),
                static_cast<Id>(elementCsv.getInt(i, "Nodes 4")),
                static_cast<Id>(elementCsv.getInt(i, "Nodes 5")),
                static_cast<Id>(elementCsv.getInt(i, "Nodes 6"))
            };
            pDb->emplace<HIElementPrism6>(id);
            auto pElem = pDb->checkOut<HIElementPrism6>(id);
            std::vector<HCursor*> nodeCursors;
            for (const auto& nodeId : nodeIds)
            {
                auto pNode = pDb->checkOut<HINode>(nodeId);
                if (pNode)
                    nodeCursors.push_back(pNode->getCursor());
            }
            pElem->setNodes(nodeCursors);
        }
        else if (elementCsv.getString(i, "Element Type") == "HEX8")
        {
            std::vector<Id> nodeIds = {
                static_cast<Id>(elementCsv.getInt(i, "Nodes 1")),
                static_cast<Id>(elementCsv.getInt(i, "Nodes 2")),
                static_cast<Id>(elementCsv.getInt(i, "Nodes 3")),
                static_cast<Id>(elementCsv.getInt(i, "Nodes 4")),
                static_cast<Id>(elementCsv.getInt(i, "Nodes 5")),
                static_cast<Id>(elementCsv.getInt(i, "Nodes 6")),
                static_cast<Id>(elementCsv.getInt(i, "Nodes 7")),
                static_cast<Id>(elementCsv.getInt(i, "Nodes 8"))
            };
            pDb->emplace<HIElementHex8>(id);
            auto pElem = pDb->checkOut<HIElementHex8>(id);
            std::vector<HCursor*> nodeCursors;
            for (const auto& nodeId : nodeIds)
            {
                auto pNode = pDb->checkOut<HINode>(nodeId);
                if (pNode)
                    nodeCursors.push_back(pNode->getCursor());
            }
            pElem->setNodes(nodeCursors);
        }
        else if (elementCsv.getString(i, "Element Type") == "QUAD4")
        {
            std::vector<Id> nodeIds = {
                static_cast<Id>(elementCsv.getInt(i, "Nodes 1")),
                static_cast<Id>(elementCsv.getInt(i, "Nodes 2")),
                static_cast<Id>(elementCsv.getInt(i, "Nodes 3")),
                static_cast<Id>(elementCsv.getInt(i, "Nodes 4"))
            };
            pDb->emplace<HIElementQuad4>(id);
            auto pElem = pDb->checkOut<HIElementQuad4>(id);
            std::vector<HCursor*> nodeCursors;
            for (const auto& nodeId : nodeIds)
            {
                auto pNode = pDb->checkOut<HINode>(nodeId);
                if (pNode)
                    nodeCursors.push_back(pNode->getCursor());
            }
            pElem->setNodes(nodeCursors);
        }
    }

    CsvTable loadCsv(loadPath.toStdString());
    for (std::size_t i = 0; i < loadCsv.rowCount(); ++i)
    {
        auto id = static_cast<Id>(i + 1);
        auto nodeId = static_cast<Id>(loadCsv.getInt(i, "Node Number"));
        auto fx = loadCsv.getDouble(i, "X Load");
        auto fy = loadCsv.getDouble(i, "Y Load");

        double fz = 0.0;
        if (!ui->checkBoxCrack->isChecked())
            fz = loadCsv.getDouble(i, "Z Load");

        pDb->emplace<HILbcForce>(id);
        auto pLbcForce = pDb->checkOut<HILbcForce>(id);
        auto pNode = pDb->checkOut<HINode>(nodeId);
        if (pLbcForce && pNode)
        {
            pLbcForce->addTarget(pNode->getCursor());
            pLbcForce->setForce({fx, fy, fz});
        }
    }

    CsvTable bcCsv(bcPath.toStdString());
    for (std::size_t i = 0; i < bcCsv.rowCount(); ++i)
    {
        auto id = static_cast<Id>(i + 1);
        auto nodeId = static_cast<Id>(bcCsv.getInt(i, "Node Number"));
        auto dofX = bcCsv.getInt(i, "X BC");
        auto dofY = bcCsv.getInt(i, "Y BC");

        int dofZ = 1;
        if (!ui->checkBoxCrack->isChecked())
            dofZ = bcCsv.getInt(i, "Z BC");

        pDb->emplace<HILbcConstraint>(id);
        auto pLbcConstraint = pDb->checkOut<HILbcConstraint>(id);
        auto pNode = pDb->checkOut<HINode>(nodeId);
        if (pLbcConstraint && pNode)
        {
            pLbcConstraint->addTarget(pNode->getCursor());
            pLbcConstraint->setDof(LbcConstraintDof::LbcConstraintDofNone);
            // Set the degrees of freedom based on the CSV data
            if (dofX == 0) pLbcConstraint->addDof(LbcConstraintDof::LbcConstraintDofTx);
            if (dofY == 0) pLbcConstraint->addDof(LbcConstraintDof::LbcConstraintDofTy);
            if (dofZ == 0) pLbcConstraint->addDof(LbcConstraintDof::LbcConstraintDofTz);
        }
    }
    pDb->commitTransaction();
}

void HornetWindow::onSolve()
{
    auto pDoc = dynamic_cast<DocumentModel*>(m_app->docs()->activeDocument());
    if (!pDoc)
        return;
    
    auto pDb = pDoc->database();
    if (!pDb)
        return;

    double youngsModulus = ui->lineEditModulus->text().toDouble();
    double poissonRatio = ui->lineEditPoison->text().toDouble();
    double density = ui->lineEditDensity->text().toDouble();
    HESolve::AnalysisType analysisType = static_cast<HESolve::AnalysisType>(ui->comboBoxAnalysis->currentData().toInt());

    if (ui->checkBoxCrack->isChecked())
    {
        double thickness = ui->lineEditThickness->text().toDouble();
        double sifRadius = ui->lineEditSifRadius->text().toDouble();
        double growthStepLength = ui->lineEditGrowthLength->text().toDouble();
        double iterations = ui->lineEditNumStep->text().toDouble();
        HESolve::ConditionType conditionType = static_cast<HESolve::ConditionType>(ui->comboBoxType->currentData().toInt());

        for (size_t i = 0; i < iterations; i++)
        {
            HESolveCrackPropagation solver(m_vecCrack, thickness, density, youngsModulus, poissonRatio, analysisType, conditionType, sifRadius, growthStepLength, pDb, static_cast<int>(i + 1));
            solver.execute();

            auto crackResult = solver.getCrackResult(); // Get results for visualization or further processing
            qDebug() << "Iteration" << i + 1 << ": Crack tip at" << crackResult[0].vCrackPoint.x << "," << crackResult[0].vCrackPoint.y;

            qDebug() << "K1" << crackResult[0].dK1;
            qDebug() << "K2" << crackResult[0].dK2;

            m_vecCrack = solver.getCrack(); // Update crack geometry for next iteration
        }
    
        ui->comboBoxStep->clear();
        ui->comboBoxStep->addItem("Initial", 0);
        for (size_t i = 0; i < iterations; i++)    {
            ui->comboBoxStep->addItem(QString("Step %1").arg(i + 1), static_cast<int>(i + 1));
        }
        ui->comboBoxStep->setCurrentIndex(0);
    }
    else
    {
        HESolveLinearAnalysis solver(density, youngsModulus, poissonRatio, analysisType, pDb);
        solver.execute();
        
        ui->comboBoxStep->clear();
        ui->comboBoxStep->addItem("Initial", 0);
        if (analysisType == HESolve::AnalysisType::Static)
        {
            ui->comboBoxStep->addItem(QString("Static"), 1);
        }
        else if (analysisType == HESolve::AnalysisType::Modal)
        {
            auto pPoolResult = pDb->getPoolUnique(CategoryType::CatResult);
            if (pPoolResult)
            {
                for (auto any : pPoolResult->range()) {
                    auto crResult = std::launder(reinterpret_cast<HCursor *>(any));
                    auto pResult = crResult->item<HIResult>();
                    if (pResult)
                    {
                        const auto& frequency = pResult->modalFrequency();
                        ui->comboBoxStep->addItem(QString("Mode %1: %2 Hz").arg(crResult->id()).arg(frequency), static_cast<int>(crResult->id()));
                    }
                }
            }
        }
        ui->comboBoxStep->setCurrentIndex(0);
    }
}

#if 0
void HornetWindow::onImportModelXfem()
{
    auto pDoc = dynamic_cast<DocumentModel*>(m_app->docs()->activeDocument());
    if (!pDoc)
        return;
    
    auto pDb = pDoc->database();
    if (!pDb)
        return;
    
    auto strDatatDir = QStringLiteral("D:\\Test\\First_BC");
    auto nodePath = QDir(strDatatDir).filePath(QStringLiteral("Node_file.csv"));
    auto elementPath = QDir(strDatatDir).filePath(QStringLiteral("Element_file.csv"));
    auto loadPath = QDir(strDatatDir).filePath(QStringLiteral("Load_file.csv"));
    auto bcPath = QDir(strDatatDir).filePath(QStringLiteral("BC_file.csv"));


    pDb->beginTransaction();

    CsvTable nodeCsv(nodePath.toStdString());
    for (std::size_t i = 0; i < nodeCsv.rowCount(); ++i)
    {
        auto id = nodeCsv.getDouble(i, "Node Number");
        auto x = nodeCsv.getDouble(i, "X Location (m)");
        auto y = nodeCsv.getDouble(i, "Y Location (m)");
        auto z = 0.0;

        pDb->emplace<HINode>(static_cast<Id>(id));
        auto pNode = pDb->checkOut<HINode>(static_cast<Id>(id));
        pNode->setPosition({x, y, z});
    }

    CsvTable elementCsv(elementPath.toStdString());
    for (std::size_t i = 0; i < elementCsv.rowCount(); ++i)
    {
        auto id = static_cast<Id>(i + 1);
        std::vector<Id> nodeIds = {
            static_cast<Id>(elementCsv.getInt(i, "Nodes 1")),
            static_cast<Id>(elementCsv.getInt(i, "Nodes 2")),
            static_cast<Id>(elementCsv.getInt(i, "Nodes 3")),
            static_cast<Id>(elementCsv.getInt(i, "Nodes 4"))
        };

        pDb->emplace<HIElementQuad4>(id);
        auto pElem = pDb->checkOut<HIElementQuad4>(id);
        std::vector<HCursor*> nodeCursors;
        for (const auto& nodeId : nodeIds)
        {
            auto pNode = pDb->checkOut<HINode>(nodeId);
            if (pNode)
                nodeCursors.push_back(pNode->getCursor());
        }
        pElem->setNodes(nodeCursors);
    }

    CsvTable loadCsv(loadPath.toStdString());
    for (std::size_t i = 0; i < loadCsv.rowCount(); ++i)
    {
        auto id = static_cast<Id>(i + 1);
        auto nodeId = static_cast<Id>(loadCsv.getInt(i, "Node Number"));
        auto fx = loadCsv.getDouble(i, "X Load");
        auto fy = loadCsv.getDouble(i, "Y Load");
        auto fz = 0.0;

        pDb->emplace<HILbcForce>(id);
        auto pLbcForce = pDb->checkOut<HILbcForce>(id);
        auto pNode = pDb->checkOut<HINode>(nodeId);
        if (pLbcForce && pNode)
        {
            pLbcForce->addTarget(pNode->getCursor());
            pLbcForce->setForce({fx, fy, fz});
        }
    }

    CsvTable bcCsv(bcPath.toStdString());
    for (std::size_t i = 0; i < bcCsv.rowCount(); ++i)
    {
        auto id = static_cast<Id>(i + 1);
        auto nodeId = static_cast<Id>(bcCsv.getInt(i, "Node Number"));
        auto dofX = bcCsv.getInt(i, "X BC");
        auto dofY = bcCsv.getInt(i, "Y BC");

        pDb->emplace<HILbcConstraint>(id);
        auto pLbcConstraint = pDb->checkOut<HILbcConstraint>(id);
        auto pNode = pDb->checkOut<HINode>(nodeId);
        if (pLbcConstraint && pNode)
        {
            pLbcConstraint->addTarget(pNode->getCursor());
            pLbcConstraint->setDof(LbcConstraintDof::LbcConstraintDofNone);
            // Set the degrees of freedom based on the CSV data
            if (dofX == 0) pLbcConstraint->addDof(LbcConstraintDof::LbcConstraintDofTx);
            if (dofY == 0) pLbcConstraint->addDof(LbcConstraintDof::LbcConstraintDofTy);
        }
    }
    pDb->commitTransaction();
}

void HornetWindow::onSolveXfem()
{
    auto pDoc = dynamic_cast<DocumentModel*>(m_app->docs()->activeDocument());
    if (!pDoc)
        return;
    
    auto pDb = pDoc->database();
    if (!pDb)
        return;

    std::vector<std::vector<HVector2d>> crack = { 
        { HVector2d(0.150, 0.0875), HVector2d(0.155, 0.0875) } 
    };

    double youngsModulus = 2e11;
    double poissonRatio = 0.3;
    double thickness = 1.0;
    double density = 1000.0;
    double sifRadius = 0.01;
    double growthStepLength = 0.005;
    double iterations = 12;

    for (size_t i = 0; i < iterations; i++)
    {
        HESolveCrackPropagation solver(crack, thickness, density, youngsModulus, poissonRatio, HESolve::AnalysisType::Static, HESolve::ConditionType::PlaneStrain, sifRadius, growthStepLength, pDb, static_cast<int>(i + 1));
        solver.execute();

        auto crackResult = solver.getCrackResult(); // Get results for visualization or further processing
        qDebug() << "Iteration" << i + 1 << ": Crack tip at" << crackResult[0].vCrackPoint.x << "," << crackResult[0].vCrackPoint.y;

        qDebug() << "K1" << crackResult[0].dK1;
        qDebug() << "K2" << crackResult[0].dK2;

        crack = solver.getCrack(); // Update crack geometry for next iteration
    }
    
    ui->comboBoxStep->clear();
    ui->comboBoxStep->addItem("Initial", 0);
    for (size_t i = 0; i < iterations; i++)    {
        ui->comboBoxStep->addItem(QString("Step %1").arg(i + 1), static_cast<int>(i + 1));
    }
    ui->comboBoxStep->setCurrentIndex(0);
}
#endif

void HornetWindow::createDocumentModel()
{
    // Setup document
    auto pDoc = m_app->docs()->createDocument<DocumentModel>();

    m_app->docs()->setActiveDocument(pDoc);
    auto pView = new GLViewWindow(nullptr);
    pDoc->setView(pView);

    pView->modelRenderer()->setShowFaces(true);
    pView->modelRenderer()->setShowEdges(true);
    pView->modelRenderer()->setShowNodes(true); // interior points off for clarity

    pView->fitView();

    pView->camera()->setFocus(QVector3D(0, 0, 0));

    pView->modelRenderer()->setElementEdgeWidth(1.0f);
    pView->modelRenderer()->setNodeSize(5.0f);                           // default point size
    pView->modelRenderer()->setDefaultElementEdgeColor(QColor("white")); // dark gray/black lines

    pView->modelRenderer()->enablePerNodeColor(true);
    pView->modelRenderer()->enablePerElementColor(true);
    pView->modelRenderer()->setDefaultElementFaceColor(QColor(190, 190, 195)); // light gray faces
    pView->modelRenderer()->setDefaultNodeColor(QColor(240, 90, 60));

    pView->coordinateGizmo()->setSize(100);          // smaller/larger corner widget
    pView->coordinateGizmo()->setMargin(0);          // distance from bottom-left
    pView->coordinateGizmo()->setScale(0.5);         // arrow thickness/length within the viewport

    // pView->setControlMode(GLViewWindow::ControlMode::FPS);
    pView->camera()->setControlMode(ControlMode::OrbitPan);

    pView->camera()->setProjection(Projection::Orthographic); // or Orthographic
    pView->camera()->setFovDegrees(10.0f);                                 // perspective FOV
    pView->camera()->setOrthoHeight(10.0f);

    pView->camera()->setOrbitRotateSpeed(10.0f);
    pView->camera()->setOrbitPanSpeed(10.0f);

    pView->lighting()->enableAmbient(true);
    pView->lighting()->enableDiffuse(true);
    pView->lighting()->enableSpecular(true);

    pView->lighting()->setAmbientIntensity(0.25f);
    pView->lighting()->setDiffuseIntensity(1.0f);
    pView->lighting()->setSpecularIntensity(0.5f);
    pView->lighting()->setShininess(48.0f);

    // Optional: set light colors (defaults are white)
    pView->lighting()->setAmbientColor(QColor(255, 255, 255));
    pView->lighting()->setDiffuseColor(QColor(255, 255, 255));
    pView->lighting()->setSpecularColor(QColor(255, 255, 255));

    // pView->clearForces();

#if 0
    std::vector<ForceArrow> vecforces;
    vecforces.reserve(100);
    for (int i = 0; i < 100; ++i)
    {
        QColor defaultColor(255, 80, 40);
        ForceArrow force;
        force.id = i + 1;
        force.position = QVector3D(i * 0.05f, 0, 0);
        force.direction = QVector3D(0, 1, 0);
        force.size = 1.0f;
        force.lengthScale = 1.0f;
        force.color = QVector4D(defaultColor.redF(), defaultColor.greenF(), defaultColor.blueF(), defaultColor.alphaF());
        force.style = ForceStyle::Tail;
        force.lightingEnabled = (i % 2 == 0);

        vecforces.emplace_back(force);
        // pView->addForceArrow(QVector3D(i * 0.05f, 0, 0),        // position
        //                                 QVector3D(0, 1, 0),                // direction
        //                                 1.0f,                              // size (screen-scale)
        //                                 1.0f,                              // lengthScale (shaft only)
        //                                 QColor(255, 80, 40),               // color
        //                                 GLViewWindow::ForceStyle::Tail,    // style
        //                                 /*lightingEnabled=*/(i % 2 == 0)); // mix lit/unlit
    }
    pView->forceRenderer()->setForces(vecforces);
#endif

    // Choose mode
    pView->selectionManager()->setType(SelectType::Element);

    // Optional tuning
    pView->setPickRadius(8);
    pView->selectionManager()->setNodeHighlightColor(QColor("#ffd700"));
    // pView->setEdgeHighlightColor(Qt::red);
    pView->selectionManager()->setElemHighlightColor(QColor(64, 160, 255));
    pView->selectionManager()->setNodeScale(1.6f);
    // pView->setHighlightEdgeScale(1.5f);

    // Alpha for highlights (0..1)
    // pView->setNodeHighlightAlpha(0.9f);
    // pView->setEdgeHighlightAlpha(0.8f);
    pView->selectionManager()->setElemAlpha(0.4f); // softer face overlay

    // Marquee colors (RGBA via QColor)
    pView->setMarqueeFillColor(QColor(64,160,255,48));     // fill
    pView->setMarqueeStrokeColor(QColor(64,160,255,160));  // outline
    pView->setMarqueeStrokeWidth(1.5f);
    
    // // Retrieve selections after user clicks/box-selects
    // const auto &selNodes = pView->selectionManager()->selectedNodeIds();
    // const auto &selElems = pView->selectionManager()->selectedElementIds();
    // const auto &selForces = pView->selectionManager()->selectedForceIds();

    m_pViewWidget = pDoc->viewWidget();
    m_pViewWidget->setParent(ui->centralwidget); // QWidget parenting for ownership/layout
    m_pViewWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    auto pLayout = qobject_cast<QBoxLayout*>(ui->centralwidget->layout());
    if (pLayout)
        pLayout->insertWidget(0, m_pViewWidget);

    // Ensure always draw
    m_pViewWidget->show();
}

void HornetWindow::onShowResult()
{
    auto pDoc = dynamic_cast<DocumentModel*>(m_app->docs()->activeDocument());
    if (!pDoc)
        return;

    auto step = ui->comboBoxStep->currentData().toInt(); // can be used to get the step number for more complex scenarios
    if (step != 0)
    {
        pDoc->view()->setStep(step);
        auto resultIdx = ui->comboBoxResultType->currentData().toInt();
        pDoc->view()->setResultComponent(resultIdx);
        pDoc->view()->setShowResultComponent(true);
        pDoc->notify(MessageType::ViewRequestRedraw);
    }
}

void HornetWindow::onUnshowResult()
{
    auto pDoc = dynamic_cast<DocumentModel*>(m_app->docs()->activeDocument());
    if (!pDoc)
        return;

    pDoc->view()->setStep(0);
    pDoc->view()->setShowDeformation(false);
    pDoc->view()->setShowResultComponent(false);
    pDoc->notify(MessageType::ViewRequestRedraw);
}

void HornetWindow::onToggleMeshLine()
{
    auto pDoc = dynamic_cast<DocumentModel*>(m_app->docs()->activeDocument());
    if (!pDoc)
        return;

    bool show = !pDoc->view()->showMeshLine();
    pDoc->view()->setShowMeshLine(show);
}

void HornetWindow::onToggleNode()
{
    auto pDoc = dynamic_cast<DocumentModel*>(m_app->docs()->activeDocument());
    if (!pDoc)
        return;

    bool show = !pDoc->view()->showNode();
    pDoc->view()->setShowNode(show);
}

void HornetWindow::onToggleLbc()
{
    auto pDoc = dynamic_cast<DocumentModel*>(m_app->docs()->activeDocument());
    if (!pDoc)
        return;

    bool show = !pDoc->view()->showLbc();
    pDoc->view()->setShowLbc(show);
}

void HornetWindow::onToggleDeformation()
{
    auto pDoc = dynamic_cast<DocumentModel*>(m_app->docs()->activeDocument());
    if (!pDoc)
        return;

    bool show = !pDoc->view()->showDeformation();
    pDoc->view()->setShowDeformation(show);
    pDoc->notify(MessageType::ViewRequestRedraw);
}

void HornetWindow::onStepChanged(int index)
{
    auto pDoc = dynamic_cast<DocumentModel*>(m_app->docs()->activeDocument());
    if (!pDoc)
        return;

    auto pDb = pDoc->database();
    if (!pDb)
        return;

    int step = ui->comboBoxStep->itemData(index).toInt();
    ui->comboBoxResultType->clear();

    if (step == 0)
        return;

    auto pResult = pDb->get<HIResult>(step);
    if (pResult)
    {
        for (int i = 0; i < pResult->getResultComponentCount(); ++i)
        {
            ui->comboBoxResultType->addItem(QString::fromStdString(pResult->getResultComponentName(i)), i);
        }

        ui->comboBoxResultType->setCurrentIndex(0);
    }
}

void HornetWindow::onEnableCrack(bool enabled)
{
    ui->lineEditSifRadius->setEnabled(enabled);
    ui->lineEditThickness->setEnabled(enabled);
    ui->lineEditGrowthLength->setEnabled(enabled);
    ui->lineEditNumStep->setEnabled(enabled);
    ui->comboBoxType->setEnabled(enabled);
}