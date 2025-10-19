#include "MainWindow.h"
#include <FancyWidgets/FancyButton.h>
#include <QVBoxLayout>
#include <QWidget>
#include <HornetView/GLViewWindow.h>
#include "ui_MainWindow.h"
#include "SecondDialog.h"
#include <QTextEdit>
#include <FancyWidgets/FOutputWidget.h>
#include <FancyWidgets/FArrowButton.h>
#include <FancyWidgets/FPanel.h>
#include <QHeaderView>
#include <FancyWidgets/FListWidgetScrollAware.h>
#include <FancyWidgets/FPropertiesPalette.h>
#include <FancyWidgets/FThemeableWidget.h>
#include <QFile>
#include <QTextStream>
#include <QStringList>
#include <HornetView/HViewDef.h>
#include <HornetBase/DatabaseSession.h>
#include <HornetBase/HINode.h>

// ---- tiny helpers ----
static inline bool isCommentOrEmpty(const QString &s)
{
    const QString t = s.trimmed();
    return t.isEmpty() || t.startsWith('#') || t.startsWith("//") || t.startsWith(';');
}
static inline float fOr(const QString &s, float def)
{
    bool ok = false;
    float v = s.toFloat(&ok);
    return ok ? v : def;
}
static inline int iOr(const QString &s, int def)
{
    bool ok = false;
    int v = s.toInt(&ok);
    return ok ? v : def;
}
static inline QStringList splitCSV(const QString &line) { return line.split(',', Qt::KeepEmptyParts); }
static inline QString lc(const QString &s) { return s.trimmed().toLower(); }

// ---- element type helpers ----
static Element::Type parseElemTypeToken(const QString &tok, bool *ok = nullptr)
{
    const QString t = lc(tok);
    if (ok)
        *ok = true;
    // 2D
    if (t == "tri3" || t == "tria3")
        return Element::Type::Tri3;
    if (t == "tri6" || t == "tria6")
        return Element::Type::Tri6;
    if (t == "quad4" || t == "quad")
        return Element::Type::Quad4;
    if (t == "quad8")
        return Element::Type::Quad8;
    // 1D
    if (t == "line2" || t == "bar2" || t == "seg2")
        return Element::Type::Line2;
    if (t == "line3" || t == "bar3" || t == "seg3")
        return Element::Type::Line3;
    // 3D
    if (t == "tet4" || t == "t4" || t == "c3d4")
        return Element::Type::Tet4;
    if (t == "tet10" || t == "t10" || t == "c3d10")
        return Element::Type::Tet10;
    if (t == "hex8" || t == "hexa8" || t == "brick8" || t == "c3d8")
        return Element::Type::Hex8;
    if (t == "hex20" || t == "hexa20" || t == "brick20" || t == "c3d20")
        return Element::Type::Hex20;
    if (ok)
        *ok = false;
    return Element::Type::Tri3; // fallback
}
static int nodeCountFor(Element::Type t)
{
    using T = Element::Type;
    switch (t)
    {
    case T::Tri3:
        return 3;
    case T::Tri6:
        return 6;
    case T::Quad4:
        return 4;
    case T::Quad8:
        return 8;
    case T::Line2:
        return 2;
    case T::Line3:
        return 3;
    case T::Tet4:
        return 4;
    case T::Tet10:
        return 10;
    case T::Hex8:
        return 8;
    case T::Hex20:
        return 20;
    }
    return 3;
}
static inline int parseNodeHeaderIndex(const QString &token)
{
    const QString t = lc(token);
    if (t.size() >= 2 && (t[0] == 'n' || t[0] == 'v'))
    {
        bool ok = false;
        int idx = t.mid(1).toInt(&ok);
        if (ok && idx >= 0 && idx <= 19)
            return idx;
    }
    return -1;
}

// ------------------------- NODES -------------------------
static bool loadNodesCSV(const QString &path, std::vector<Node> &out)
{
    out.clear();
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    QTextStream ts(&f);
    bool header = false;
    int idCol = -1, xCol = -1, yCol = -1, zCol = -1, rCol = -1, gCol = -1, bCol = -1, aCol = -1;
    while (!ts.atEnd())
    {
        const QString raw = ts.readLine();
        if (isCommentOrEmpty(raw))
            continue;
        const QStringList parts = splitCSV(raw.trimmed());
        if (!header)
        {
            bool looksHeader = false;
            for (const QString &p : parts)
            {
                bool isNum = false;
                p.toDouble(&isNum);
                if (!isNum)
                {
                    looksHeader = true;
                    break;
                }
            }
            if (looksHeader)
            {
                for (int i = 0; i < parts.size(); ++i)
                {
                    const QString k = lc(parts[i]);
                    if (k == "id")
                        idCol = i;
                    else if (k == "x")
                        xCol = i;
                    else if (k == "y")
                        yCol = i;
                    else if (k == "z")
                        zCol = i;
                    else if (k == "r")
                        rCol = i;
                    else if (k == "g")
                        gCol = i;
                    else if (k == "b")
                        bCol = i;
                    else if (k == "a")
                        aCol = i;
                }
                header = true;
                continue;
            }
            header = true; // first row is data
        }
        Node n;
        auto get = [&](int c) -> QString
        { return (c >= 0 && c < parts.size()) ? parts[c] : QString(); };
        if (idCol < 0 && xCol < 0 && yCol < 0 && zCol < 0)
        {
            if (parts.size() < 4)
                continue;
            n.id = iOr(parts[0], 0);
            n.x = fOr(parts[1], 0.f);
            n.y = fOr(parts[2], 0.f);
            n.z = fOr(parts[3], 0.f);
            if (parts.size() >= 8)
            {
                n.r = fOr(parts[4], 0.95f);
                n.g = fOr(parts[5], 0.35f);
                n.b = fOr(parts[6], 0.25f);
                n.a = fOr(parts[7], 1.0f);
            }
            else
            {
                n.r = 0.95f;
                n.g = 0.35f;
                n.b = 0.25f;
                n.a = 1.0f;
            }
        }
        else
        {
            n.id = iOr(get(idCol), 0);
            n.x = fOr(get(xCol), 0.f);
            n.y = fOr(get(yCol), 0.f);
            n.z = fOr(get(zCol), 0.f);
            n.r = (rCol >= 0) ? fOr(get(rCol), 0.95f) : 0.95f;
            n.g = (gCol >= 0) ? fOr(get(gCol), 0.35f) : 0.35f;
            n.b = (bCol >= 0) ? fOr(get(bCol), 0.25f) : 0.25f;
            n.a = (aCol >= 0) ? fOr(get(aCol), 1.0f) : 1.0f;
        }
        out.push_back(n);
    }
    return true;
}

// ----------------------- ELEMENTS -----------------------
static bool loadElementsCSV(const QString &path, std::vector<Element> &out)
{
    out.clear();
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    QTextStream ts(&f);
    bool header = false;
    int typeCol = -1, rCol = -1, gCol = -1, bCol = -1, aCol = -1;
    QVector<int> nodeCols;
    nodeCols.resize(20, -1);
    while (!ts.atEnd())
    {
        const QString raw = ts.readLine();
        if (isCommentOrEmpty(raw))
            continue;
        const QStringList parts = splitCSV(raw.trimmed());
        if (!header)
        {
            bool looksHeader = false;
            for (const QString &p : parts)
            {
                const QString t = lc(p);
                if (t == "type" || t == "r" || t == "g" || t == "b" || t == "a" || parseNodeHeaderIndex(t) >= 0)
                {
                    looksHeader = true;
                    break;
                }
            }
            if (looksHeader)
            {
                for (int i = 0; i < parts.size(); ++i)
                {
                    const QString t = lc(parts[i]);
                    if (t == "type")
                        typeCol = i;
                    else if (t == "r")
                        rCol = i;
                    else if (t == "g")
                        gCol = i;
                    else if (t == "b")
                        bCol = i;
                    else if (t == "a")
                        aCol = i;
                    else
                    {
                        int idx = parseNodeHeaderIndex(t);
                        if (idx >= 0)
                        {
                            if (idx >= nodeCols.size())
                                nodeCols.resize(idx + 1, -1);
                            nodeCols[idx] = i;
                        }
                    }
                }
                header = true;
                continue;
            }
            header = true; // first row is data
        }

        Element e; // uses defaults in your struct
        if (typeCol >= 0)
        {
            bool okT = false;
            e.type = parseElemTypeToken(parts.value(typeCol), &okT);
            const int need = nodeCountFor(e.type);
            for (int i = 0; i < need; ++i)
            {
                int col = (i < nodeCols.size()) ? nodeCols[i] : -1;
                e.v[i] = iOr(parts.value(col), 0);
            }
            if (rCol >= 0)
                e.r = fOr(parts.value(rCol), e.r);
            if (gCol >= 0)
                e.g = fOr(parts.value(gCol), e.g);
            if (bCol >= 0)
                e.b = fOr(parts.value(bCol), e.b);
            if (aCol >= 0)
                e.a = fOr(parts.value(aCol), e.a);
            out.push_back(e);
            continue;
        }

        // no-header path
        int start = 0;
        bool okT = false;
        auto t = parseElemTypeToken(parts.first(), &okT);
        if (okT)
            start = 1;
        const int remain = parts.size() - start;
        if (!okT)
        {
            if (remain == 2)
                t = Element::Type::Line2;
            else if (remain == 3)
                t = Element::Type::Tri3;
            else if (remain == 4)
                t = Element::Type::Quad4; // Tet4 ambiguous → require token
            else if (remain == 6)
                t = Element::Type::Tri6;
            else if (remain == 8)
                t = Element::Type::Quad8; // Hex8 ambiguous → require token
            else if (remain == 10)
                t = Element::Type::Tet10;
            else if (remain == 20)
                t = Element::Type::Hex20;
            else
                t = Element::Type::Tri3;
        }
        e.type = t;
        const int need = nodeCountFor(t);
        if (remain < need)
            continue;
        for (int i = 0; i < need; ++i)
            e.v[i] = iOr(parts[start + i], 0);
        if (remain >= need + 1)
            e.r = fOr(parts[start + need + 0], e.r);
        if (remain >= need + 2)
            e.g = fOr(parts[start + need + 1], e.g);
        if (remain >= need + 3)
            e.b = fOr(parts[start + need + 2], e.b);
        if (remain >= need + 4)
            e.a = fOr(parts[start + need + 3], e.a);
        out.push_back(e);
    }
    return true;
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    // Init data
    initTheme();

    // Remove native title bar
    setWindowFlags(Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_TranslucentBackground, false);

    // Setup UI
    ui->setupUi(this);

    // connect(ui->fancyButton, &QPushButton::clicked, this, []() {
    //     SecondDialog dialog;
    //     dialog.exec();
    // });

    connect(ui->fancyButton, &QPushButton::clicked, this, &MainWindow::on_fancyButton_clicked);

    // Test
    ui->horizontalLayout->setContentsMargins(4, 0, 4, 4);
    ui->horizontalLayout->setSpacing(4);
    ui->sideBar->readJson(":/HornetMain/res/template/SideBar.json");

    ui->sideBar->createTab("Assemble", ":/HornetMain/res/icon/unfilled/portrait.png", ":/HornetMain/res/icon/filled/portrait.png", createTreeWidget, true);

    ui->titleBar->setAppTitleIcon("", ":/HornetMain/res/icon/custom/bee.png");
    ui->tabBar->setFixedHeight(200);
    ui->tabBar->addTabBar(100);
    ui->tabBar->addTabBar(100);

    for (auto i = 0; i < 6; i++)
    {
        FOutputWidget *output = new FOutputWidget(this);
        output->enableInput(true); // Enable input area
        output->appendMessage("Info: Build started", Qt::white);
        output->appendMessage("Warning: Missing include", Qt::yellow);
        output->appendMessage("Error: Failed to compile", Qt::red);

        connect(output, &FOutputWidget::userInputSubmitted, this, [](const QString &cmd)
                {
                    qDebug() << "User typed:" << cmd;
                    // Handle commands or routing here
                });

        QString title = QString("Output %1").arg(QString::number(i));
        if (i > 2)
            ui->tabBar->addPane(1, title, output);
        else
            ui->tabBar->addPane(0, title, output);
    }

    ui->glViewWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    ui->glViewWidget->modelRenderer()->setShowFaces(true);
    ui->glViewWidget->modelRenderer()->setShowEdges(true);
    ui->glViewWidget->modelRenderer()->setShowNodes(true); // interior points off for clarity

    // std::vector<Node> nodes;
    // std::vector<Element> elems;

    // const QString nodesCsv = "D:/Test/nodes_mixed_shapes_clean.csv";
    // const QString elemsCsv = "D:/Test/elements_mixed_shapes_clean.csv";

    // // If you place the downloaded CSVs next to your exe, use the above.
    // // Otherwise, set absolute paths to where you saved them.

    // if (!loadNodesCSV(nodesCsv, nodes))
    // {
    //     qWarning("Failed to load nodes: %s", qPrintable(nodesCsv));
    // }
    // if (!loadElementsCSV(elemsCsv, elems))
    // {
    //     qWarning("Failed to load elements: %s", qPrintable(elemsCsv));
    // }

    // ui->glViewWidget->setMesh(nodes, elems);
    ui->glViewWidget->fitView();

    ui->glViewWidget->camera()->setFocus(QVector3D(0, 0, 0));

    ui->glViewWidget->modelRenderer()->setElementEdgeWidth(1.0f);
    ui->glViewWidget->modelRenderer()->setNodeSize(5.0f);                           // default point size
    ui->glViewWidget->modelRenderer()->setDefaultElementEdgeColor(QColor("white")); // dark gray/black lines

    ui->glViewWidget->modelRenderer()->enablePerNodeColor(true);
    ui->glViewWidget->modelRenderer()->enablePerElementColor(true);
    ui->glViewWidget->modelRenderer()->setDefaultElementFaceColor(QColor(190, 190, 195)); // light gray faces
    ui->glViewWidget->modelRenderer()->setDefaultNodeColor(QColor(240, 90, 60));

    ui->glViewWidget->coordinateGizmo()->setSize(100);          // smaller/larger corner widget
    ui->glViewWidget->coordinateGizmo()->setMargin(0);          // distance from bottom-left
    ui->glViewWidget->coordinateGizmo()->setScale(0.5);         // arrow thickness/length within the viewport

    // ui->glViewWidget->setControlMode(GLViewWindow::ControlMode::FPS);
    ui->glViewWidget->camera()->setControlMode(ControlMode::OrbitPan);

    ui->glViewWidget->camera()->setProjection(Projection::Orthographic); // or Orthographic
    ui->glViewWidget->camera()->setFovDegrees(10.0f);                                 // perspective FOV
    ui->glViewWidget->camera()->setOrthoHeight(10.0f);

    ui->glViewWidget->camera()->setOrbitRotateSpeed(30.0f);
    ui->glViewWidget->camera()->setOrbitPanSpeed(30.0f);

    ui->glViewWidget->lighting()->enableAmbient(true);
    ui->glViewWidget->lighting()->enableDiffuse(true);
    ui->glViewWidget->lighting()->enableSpecular(true);

    ui->glViewWidget->lighting()->setAmbientIntensity(0.25f);
    ui->glViewWidget->lighting()->setDiffuseIntensity(1.0f);
    ui->glViewWidget->lighting()->setSpecularIntensity(0.5f);
    ui->glViewWidget->lighting()->setShininess(48.0f);

    // Optional: set light colors (defaults are white)
    ui->glViewWidget->lighting()->setAmbientColor(QColor(255, 255, 255));
    ui->glViewWidget->lighting()->setDiffuseColor(QColor(255, 255, 255));
    ui->glViewWidget->lighting()->setSpecularColor(QColor(255, 255, 255));

    // ui->glViewWidget->clearForces();

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
        // ui->glViewWidget->addForceArrow(QVector3D(i * 0.05f, 0, 0),        // position
        //                                 QVector3D(0, 1, 0),                // direction
        //                                 1.0f,                              // size (screen-scale)
        //                                 1.0f,                              // lengthScale (shaft only)
        //                                 QColor(255, 80, 40),               // color
        //                                 GLViewWindow::ForceStyle::Tail,    // style
        //                                 /*lightingEnabled=*/(i % 2 == 0)); // mix lit/unlit
    }
    ui->glViewWidget->forceRenderer()->setForces(vecforces);


    // Choose mode
    ui->glViewWidget->selectionManager()->setType(SelectType::Element);

    // Optional tuning
    ui->glViewWidget->setPickRadius(8);
    ui->glViewWidget->selectionManager()->setNodeHighlightColor(QColor("#ffd700"));
    // ui->glViewWidget->setEdgeHighlightColor(Qt::red);
    ui->glViewWidget->selectionManager()->setElemHighlightColor(QColor(64, 160, 255));
    ui->glViewWidget->selectionManager()->setNodeScale(1.6f);
    // ui->glViewWidget->setHighlightEdgeScale(1.5f);

    // Alpha for highlights (0..1)
    // ui->glViewWidget->setNodeHighlightAlpha(0.9f);
    // ui->glViewWidget->setEdgeHighlightAlpha(0.8f);
    ui->glViewWidget->selectionManager()->setElemAlpha(0.4f); // softer face overlay

    // Marquee colors (RGBA via QColor)
    ui->glViewWidget->setMarqueeFillColor(QColor(64,160,255,48));     // fill
    ui->glViewWidget->setMarqueeStrokeColor(QColor(64,160,255,160));  // outline
    ui->glViewWidget->setMarqueeStrokeWidth(1.5f);
    
    // // Retrieve selections after user clicks/box-selects
    // const auto &selNodes = ui->glViewWidget->selectionManager()->selectedNodeIds();
    // const auto &selElems = ui->glViewWidget->selectionManager()->selectedElementIds();
    // const auto &selForces = ui->glViewWidget->selectionManager()->selectedForceIds();

    // Connect signal
    connect(ui->titleBar, &FTitleBar::signalClose, this, &MainWindow::close);
    connect(ui->titleBar, &FTitleBar::signalMinimize, [&]() {
        this->showMinimized();
    });
    connect(ui->titleBar, &FTitleBar::signalToggleMaximize, [&]() {
        static bool maximized = false;
        maximized = !maximized;
        maximized ? this->showMaximized() : this->showNormal();
    });


    /// test database
    DatabaseSession db;

    db.beginTransaction();
    db.emplace<HINode>(1);
    db.emplace<HINode>(2);
    db.emplace<HINode>(3);
    db.emplace<HINode>(4);
    db.emplace<HINode>(5);

    db.emplace<HIElemTest>(6);
    db.emplace<HIElemTest>(7);
    db.commitTransaction();

    auto stats = db.stats();
    qDebug() << "categories=" << stats.store_count
             << " object=" << stats.objects
             << " bytes_used~" << stats.bytes_used << "\n";

    if (auto *a = db.get<HINode>(1))
    {
        qDebug() << a->id() << "\n";
    }
    if (auto *b = db.get<HINode>(5))
    {
        qDebug() << b->id() << "\n";
    }

    db.beginTransaction();
    // Delete A(1) — this will compact that chunk
    db.erase<HIElemTest>(6);
    db.commitTransaction();

    // Iterate A in memory order
    // db.for_each_in_memory_order<A>([](Id id, A& a) { /* ... */ });
    auto stats2 = db.stats();
    qDebug() << "categories=" << stats2.store_count
             << " object=" << stats2.objects
             << " bytes_used~" << stats2.bytes_used << "\n";

    db.beginTransaction();
    db.emplace<HIElemTest>(11);
    db.emplace<HIElemTest>(12);
    db.commitTransaction();

    auto stats3 = db.stats();
    qDebug() << "categories=" << stats3.store_count
             << " object=" << stats3.objects
             << " bytes_used~" << stats3.bytes_used << "\n";

    auto *noMix = db.getPoolUnique(CategoryType::CatNode);
    if (noMix)
    {
        for (auto any : noMix->range())
        {
            auto item = std::launder(reinterpret_cast<HCursor *>(any));
            qDebug() << "No mix id is =" << item->id() << "\n";
        }
    }

    auto *doMix = db.getPoolMix(CategoryType::CatElement);
    if (doMix)
    {
        for (auto any : doMix->range())
        {
            auto item2 = std::launder(reinterpret_cast<HCursor *>(any));
            qDebug() << "Do mix id is =" << item2->id() << "\n";
        }
    }

    auto pTestItem = db.get<HINode>(3);
    if (pTestItem)
    {
        auto testcursor = pTestItem->getCursor();
    }
    
    db.undo();
    db.undo();

    auto stats4 = db.stats();
    qDebug() << "categories=" << stats4.store_count
             << " object=" << stats4.objects
             << " bytes_used~" << stats4.bytes_used << "\n";

    auto *noMix2 = db.getPoolUnique(CategoryType::CatNode);
    if (noMix2)
    {
        for (auto cursor : noMix2->range())
        {
            qDebug() << "No mix id is =" << cursor->id() << "\n";
        }
    }

    auto *doMix2 = db.getPoolMix(CategoryType::CatElement);
    if (doMix2)
    {
        for (auto cursor : doMix2->range())
        {
            qDebug() << "Do mix id is =" << cursor->id() << "\n";
        }
    }
    db.redo();
    auto stats5 = db.stats();
    qDebug() << "categories=" << stats5.store_count
             << " object=" << stats5.objects
             << " bytes_used~" << stats5.bytes_used << "\n";

    auto *noMix3 = db.getPoolUnique(CategoryType::CatNode);
    if (noMix3)
    {
        for (auto cursor : noMix3->range())
        {
            qDebug() << "No mix id is =" << cursor->id() << "\n";
        }
    }

    auto *doMix3 = db.getPoolMix(CategoryType::CatElement);
    if (doMix3)
    {
        for (auto cursor : doMix3->range())
        {
            qDebug() << "Do mix id is =" << cursor->id() << "\n";
        }
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initTheme()
{
    FThemeManager::instance().load(":/HornetMain/res/theme/Themes.json", ":/HornetMain/res/theme/NumericDisplay.json");
    FThemeManager::instance().setTheme("Dark");
}

QWidget* MainWindow::createTreeWidget()
{
    auto pSplitter = new FSplitter(Qt::Vertical);
    pSplitter->setHandleSeparatorOffset(0, 4);
    pSplitter->setThemeStyle(FThemeableWidget::Custom1);

    pSplitter->setHandleWidth(2);
    pSplitter->setHandleSpacing(2);

    auto pScroll1 = new FListWidgetScrollAware(pSplitter);
    pScroll1->enableSpacingBorder(false);

    FTreeWidget *tree = new FTreeWidget(pScroll1);
    populateTree(tree);
    pScroll1->addWidget(tree);
    pScroll1->setSpacing(4);

    FPropertiesPalette *treeView = new FPropertiesPalette(pSplitter);
    treeView->setHeaderLabels({"Name", "Type"});
    treeView->setColumnCount(2); // One column: we customize item content fully
    auto pTest1 = treeView->addItem("EEEEEEEEEE", {"test1"});
    auto pTest3 = treeView->addItem("EEEEEEEEEE", {"test3"});

    treeView->addItem("EEEEEEEE", {"test12"}, pTest1);
    treeView->addItem("test13", {"test13"}, pTest1);
    auto pChild4 = treeView->addItem("test14", {"test14"}, pTest1);
    treeView->addItem("test141", {"test141"}, pChild4);
    treeView->addItem("test142", {"test142"}, pChild4);
    auto pChild6 = treeView->addItem("test16", {"test16"}, pTest1);
    treeView->addItem("test161", {"test161"}, pChild6);
    treeView->addItem("test162", {"test162"}, pChild6);
    auto pChild7 = treeView->addItem("test17", {"test17"}, pTest1);
    treeView->addItem("test171", {"test161"}, pChild7);
    treeView->addItem("test172", {"test162"}, pChild7);
    treeView->addItem("test15", {"test15"}, pTest1);
    auto pChild8 = treeView->addItem("test18", {"test18"}, pTest1);
    treeView->addItem("test171", {"test161"}, pChild8);
    treeView->addItem("test172", {"test162"}, pChild8);

    treeView->addItem("test32", {"test32"}, pTest3);
    treeView->addItem("test33", {"test33"}, pTest3);
    auto listChild = treeView->topLevelItemCount();
    pSplitter->addWidget(pScroll1);
    // pSplitter->addWidget(pScroll2);
    pSplitter->addWidget(treeView);

   
    return pSplitter;
}

void MainWindow::addRowToTable(FTable *table)
{
    if (!table)
        return;

    int row = table->rowCount();
    table->insertRow(row);

    for (int i = 0; i < 2; ++i)
    {
        if (i < table->columnCount())
        {
            auto pLabel = new QLabel(QString("Item %1-%2").arg(row, i), table);
            pLabel->setStyleSheet("QLabel { background: transparent; }");
            table->setCellWidget(row, i, pLabel);
        }
    }
}

void MainWindow::populateTree(FTreeWidget *tree)
{
    auto createLabel = [](const QString &text) -> QWidget *
    {
        FTreeItem *label = new FTreeItem(text, "D:\\Icon\\Apple\\bee.png");
        // auto label = new QPushButton(text);
        return label;
    };

    auto createCheckBox = []() -> QWidget *
    {
        FCheckBox *label = new FCheckBox();
        return label;
    };

    QList<QWidget*> listWidget;
    listWidget.append(createLabel("Desk 1"));
    // listWidget.append(createCheckBox());
    auto child1 = tree->addItem(nullptr, listWidget);

    QList<QWidget *> listWidget2;
    listWidget2.append(createLabel("1 - New Business"));
    // listWidget2.append(createCheckBox());
    auto child2 = tree->addItem(child1, listWidget2);

    QList<QWidget *> listWidget3;
    listWidget3.append(createLabel("2 - Contacted"));
    // listWidget3.append(createCheckBox());
    auto child3 = tree->addItem(child1, listWidget3);

    QList<QWidget *> listWidget4;
    listWidget4.append(createLabel("Non-Compliant"));
    // listWidget4.append(createCheckBox());
    auto child4 = tree->addItem(child3, listWidget4);

    QList<QWidget *> listWidget5;
    listWidget5.append(createLabel("Compliant"));
    // listWidget5.append(createCheckBox());
    auto child5 = tree->addItem(child3, listWidget5);

    QList<QWidget *> listWidget6;
    listWidget6.append(createLabel("Desk 2"));
    // listWidget6.append(createCheckBox());
    auto child6 = tree->addItem(nullptr, listWidget6);

    QList<QWidget *> listWidget7;
    listWidget7.append(createLabel("Desk 2"));
    // listWidget7.append(createCheckBox());
    auto child7 = tree->addItem(child6, listWidget7);

    // auto createLabel = [](const QString &text) -> QWidget *
    // {
    //     QLabel *label = new QLabel(text);
    //     return label;
    // };

    // // Top-level item: My Computer
    // QTreeWidgetItem *computer = new QTreeWidgetItem(tree);
    // tree->setItemWidget(computer, 0, createLabel("My Computer"));
    // tree->setItemWidget(computer, 1, createLabel("System"));
    // tree->setItemWidget(computer, 2, createLabel("-"));

    // // Child: Documents
    // QTreeWidgetItem *documents = new QTreeWidgetItem(computer);
    // tree->setItemWidget(documents, 0, createLabel("Documents"));
    // tree->setItemWidget(documents, 1, createLabel("Folder"));
    // tree->setItemWidget(documents, 2, createLabel("-"));

    // // Child of Documents: resume.pdf
    // QTreeWidgetItem *resume = new QTreeWidgetItem(documents);
    // tree->setItemWidget(resume, 0, createLabel("resume.pdf"));
    // tree->setItemWidget(resume, 1, createLabel("PDF File"));
    // tree->setItemWidget(resume, 2, createLabel("245 KB"));

    // // Another top-level item: Downloads
    // QTreeWidgetItem *downloads = new QTreeWidgetItem(tree);
    // tree->setItemWidget(downloads, 0, createLabel("Downloads"));
    // tree->setItemWidget(downloads, 1, createLabel("Folder"));
    // tree->setItemWidget(downloads, 2, createLabel("-"));

    // // Child: installer.exe
    // QTreeWidgetItem *installer = new QTreeWidgetItem(downloads);
    // tree->setItemWidget(installer, 0, createLabel("installer.exe"));
    // tree->setItemWidget(installer, 1, createLabel("Executable"));
    // tree->setItemWidget(installer, 2, createLabel("15 MB"));
}

void MainWindow::on_fancyButton_clicked()
{
    std::vector<Node> nodes;
    std::vector<Element> elems;

    const QString nodesCsv = "D:/Test/nodes_mixed_shapes_clean.csv";
    const QString elemsCsv = "D:/Test/elements_mixed_shapes_clean.csv";

    // If you place the downloaded CSVs next to your exe, use the above.
    // Otherwise, set absolute paths to where you saved them.

    if (!loadNodesCSV(nodesCsv, nodes))
    {
        qWarning("Failed to load nodes: %s", qPrintable(nodesCsv));
    }
    if (!loadElementsCSV(elemsCsv, elems))
    {
        qWarning("Failed to load elements: %s", qPrintable(elemsCsv));
    }

    ui->glViewWidget->setMesh(nodes, elems);
    ui->glViewWidget->fitView();
}