#define AEROTRACK_TABS_LAYOUT
#include "mainwindow.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QWidget>
#include <QColor>
#include <QStatusBar>
#include <QFrame>
#include <QTimer>
#include <QTextStream>
#include <QPainter>
#include <QPainterPath>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QSpinBox>
#include <QTabWidget>
#include <QScrollArea>
#include <QButtonGroup>
#include <QListWidget>
#include <QScrollBar>
#include <QMouseEvent>
#include <QLineF>
#include <functional>
#include <cmath>
#include <QDateTime>

namespace {
const QStringList AIRLINES = {"IndiGo", "Air India", "Air India Express", "SpiceJet",
                              "Akasa Air", "Alliance Air", "Star Air"};
const QStringList CODES = {"6E", "AI", "IX", "SG", "QP", "9I", "S5"};
const QStringList STATUSES = {"Checked In", "In Transit", "Arrived", "Delivered", "Lost"};
const QStringList STATUS_COLORS = {"#60a5fa", "#fbbf24", "#c4b5fd", "#34d399", "#fb7185"};

QString dataPath() {
    return QDir(QCoreApplication::applicationDirPath()).filePath("baggage_data.json");
}
QLabel *label(const QString &text, const QString &name = {}) {
    auto *l = new QLabel(text);
    if (!name.isEmpty()) l->setObjectName(name);
    return l;
}
QFrame *panel() {
    auto *f = new QFrame;
    f->setObjectName("panel");
    return f;
}

class AnalyticsChart : public QWidget {
public:
    enum Mode { StatusPie, AirlineBars };
    AnalyticsChart(const QList<Baggage> *data, Mode mode, QWidget *parent = nullptr)
        : QWidget(parent), data_(data), mode_(mode) {
        setMinimumHeight(260);
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        setCursor(Qt::PointingHandCursor);
        setToolTip(mode_ == StatusPie ? "Click a status slice to filter the manifest" : "Click an airline bar to filter the manifest");
    }
    void setCategoryCallback(std::function<void(const QString&)> callback) { categoryCallback_ = std::move(callback); }
protected:
    void mousePressEvent(QMouseEvent *event) override {
        if (event->button() != Qt::LeftButton || !categoryCallback_) { QWidget::mousePressEvent(event); return; }
        QRect area = rect().adjusted(16, 16, -16, -16);
        area.setTop(area.top() + 36);
        if (mode_ == StatusPie) {
            int counts[5] = {};
            for (const auto &r : *data_) { int i = STATUSES.indexOf(r.status); if (i >= 0) counts[i]++; }
            int total = 0; for (int c : counts) total += c;
            int diameter = qMax(1, qMin(area.height() - 8, area.width() / 2));
            QRect pie(area.left() + 8, area.top() + 4, diameter, diameter);
            const QPointF center = pie.center();
            const QPointF pos = event->position();
            if (total > 0 && QLineF(center, pos).length() <= diameter / 2) {
                double angle = std::atan2(-(pos.y() - center.y()), pos.x() - center.x()) * 180.0 / 3.14159265358979323846;
                if (angle < 0) angle += 360.0;
                double cumulative = 0.0;
                for (int i = 0; i < 5; ++i) {
                    if (counts[i] == 0) continue;
                    const double span = 360.0 * counts[i] / total;
                    if (angle >= cumulative && angle < cumulative + span) { categoryCallback_(STATUSES[i]); return; }
                    cumulative += span;
                }
            }
        } else {
            const int left = area.left() + 8;
            const int slot = qMax(30, (area.width() - 20) / 7);
            const int idx = (event->pos().x() - left) / slot;
            if (idx >= 0 && idx < AIRLINES.size()) { categoryCallback_(AIRLINES[idx]); return; }
        }
        QWidget::mousePressEvent(event);
    }
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.fillRect(rect(), QColor("#111c30"));
        QRect area = rect().adjusted(16, 16, -16, -16);
        p.setPen(QColor("#e5edf8"));
        p.setFont(QFont("Segoe UI", 11, QFont::Bold));
        p.drawText(QRect(area.left(), area.top(), area.width(), 26),
                   Qt::AlignLeft | Qt::AlignVCenter,
                   mode_ == StatusPie ? "BAGGAGE STATUS MIX" : "BAGS BY AIRLINE");
        area.setTop(area.top() + 36);
        if (mode_ == StatusPie) drawPie(p, area); else drawBars(p, area);
    }
private:
    const QList<Baggage> *data_;
    Mode mode_;
    std::function<void(const QString&)> categoryCallback_;
    void drawPie(QPainter &p, QRect area) {
        int counts[5] = {};
        for (const auto &r : *data_) {
            int i = STATUSES.indexOf(r.status);
            if (i >= 0) counts[i]++;
        }
        int total = 0;
        for (int c : counts) total += c;
        int diameter = qMax(1, qMin(area.height() - 8, area.width() / 2));
        QRect pie(area.left() + 8, area.top() + 4, diameter, diameter);
        if (total == 0) {
            p.setPen(QColor("#91a5c3"));
            p.drawText(pie, Qt::AlignCenter, "No baggage data yet");
        } else {
            int start = 0;
            for (int i = 0; i < 5; ++i) {
                if (counts[i] == 0) continue;
                int span = qRound(360.0 * counts[i] / total * 16);
                p.setBrush(QColor(STATUS_COLORS[i]));
                p.setPen(QPen(QColor("#111c30"), 2));
                p.drawPie(pie, start, span);
                start += span;
            }
        }
        int lx = pie.right() + 22, ly = area.top() + 12;
        p.setFont(QFont("Segoe UI", 9));
        for (int i = 0; i < 5; ++i) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(STATUS_COLORS[i]));
            p.drawRoundedRect(QRect(lx, ly + i * 34, 12, 12), 3, 3);
            p.setPen(QColor("#cbd5e1"));
            QString line = QString("%1   %2 (%3%)").arg(STATUSES[i]).arg(counts[i])
                               .arg(total ? qRound(100.0 * counts[i] / total) : 0);
            p.drawText(lx + 22, ly + i * 34 + 11, line);
        }
    }
    void drawBars(QPainter &p, QRect area) {
        int counts[7] = {};
        for (const auto &r : *data_) {
            int i = AIRLINES.indexOf(r.airline);
            if (i >= 0) counts[i]++;
        }
        int maxVal = 1;
        for (int c : counts) maxVal = qMax(maxVal, c);
        int left = area.left() + 8, bottom = area.bottom() - 32;
        int chartH = qMax(80, area.height() - 65);
        int slot = qMax(30, (area.width() - 20) / 7);
        p.setFont(QFont("Segoe UI", 8));
        for (int i = 0; i < 7; ++i) {
            int bh = qRound((double)counts[i] / maxVal * chartH);
            int x = left + i * slot + 6;
            QRect bar(x, bottom - bh, qMax(12, slot - 16), bh);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(i % 2 ? "#22d3ee" : "#3b82f6"));
            p.drawRoundedRect(bar, 4, 4);
            p.setPen(QColor("#e5edf8"));
            p.drawText(QRect(x - 4, bottom - bh - 22, slot, 18), Qt::AlignHCenter, QString::number(counts[i]));
            p.setPen(QColor("#91a5c3"));
            QString shortName = AIRLINES[i];
            if (shortName == "Air India Express") shortName = "AI Express";
            if (shortName == "Alliance Air") shortName = "Alliance";
            p.drawText(QRect(left + i * slot - 6, bottom + 7, slot + 4, 28),
                       Qt::AlignHCenter | Qt::AlignTop, shortName);
        }
        p.setPen(QPen(QColor("#2b3d5b"), 1));
        p.drawLine(left, bottom, area.right() - 4, bottom);
    }
};
}

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
#ifdef AEROTRACK_TABS_LAYOUT
    tabsLayout = true;
#endif
    buildUi();
    applyStyle();
    loadFromFile();
    refreshAll();
    auto *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this] {
        clockLabel->setText(QDateTime::currentDateTime().toString("ddd, dd MMM yyyy  |  hh:mm:ss"));
    });
    timer->start(1000);
    clockLabel->setText(QDateTime::currentDateTime().toString("ddd, dd MMM yyyy  |  hh:mm:ss"));
}
MainWindow::~MainWindow() = default;

void MainWindow::buildUi() {
    setWindowTitle("AeroTrack | Airport Baggage Intelligence");
    resize(1360, 900);
    setMinimumSize(980, 680);

    auto *central = new QWidget;
    auto *outer = new QVBoxLayout(central);
    outer->setContentsMargins(24, 20, 24, 16);
    outer->setSpacing(12);

    auto *header = new QGridLayout;
    header->setColumnStretch(0, 1);
    header->setColumnStretch(1, 0);
    header->setColumnStretch(2, 1);
    auto *leftBrand = new QHBoxLayout;
    leftBrand->setSpacing(10);
    auto *brandMark = label("✈", "brandMark");
    brandMark->setAlignment(Qt::AlignCenter);
    brandMark->setFixedSize(48, 48);
    auto *brandText = new QVBoxLayout;
    brandText->setSpacing(2);
    brandText->addWidget(label("AEROTRACK", "brandName"));
    brandText->addWidget(label("SECURE BAGGAGE OPS", "brandCaption"));
    leftBrand->addWidget(brandMark);
    leftBrand->addLayout(brandText);
    leftBrand->addStretch(1);
    header->addLayout(leftBrand, 0, 0, Qt::AlignLeft | Qt::AlignVCenter);

    auto *brand = new QVBoxLayout;
    auto *eyebrow = label("AEROTRACK  /  AIRPORT OPERATIONS", "eyebrow");
    auto *pageTitle = label("Baggage Intelligence", "pageTitle");
    auto *subtitle = label("Airport baggage monitoring, journey tracking & operations", "subtitle");
    eyebrow->setAlignment(Qt::AlignCenter);
    pageTitle->setAlignment(Qt::AlignCenter);
    subtitle->setAlignment(Qt::AlignCenter);
    brand->setSpacing(5);
    brand->addWidget(eyebrow);
    brand->addWidget(pageTitle);
    brand->addWidget(subtitle);
    header->addLayout(brand, 0, 1, Qt::AlignHCenter | Qt::AlignVCenter);
    auto *right = new QVBoxLayout;
    auto *live = label("●  SYSTEM ONLINE", "livePill");
    live->setAlignment(Qt::AlignCenter);
    clockLabel = label("", "clockLabel");
    clockLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    right->addWidget(live);
    right->addWidget(clockLabel);
    header->addLayout(right, 0, 2, Qt::AlignRight | Qt::AlignTop);
    outer->addLayout(header);

    auto *stats = new QHBoxLayout;
    stats->setSpacing(10);
    auto makeStat = [stats](QString title, QString obj, QString color) {
        auto *card = new QFrame;
        card->setObjectName("statCard");
        auto *v = new QVBoxLayout(card);
        v->setContentsMargins(14, 10, 14, 10);
        v->setSpacing(5);
        v->addWidget(label(title, "statCaption"));
        auto *value = label("0", obj);
        value->setStyleSheet(QString("color:%1; background:transparent;").arg(color));
        v->addWidget(value);
        stats->addWidget(card, 1);
        return value;
    };
    totalValue = makeStat("TOTAL RECORDS", "totalValue", "#f8fafc");
    checkedValue = makeStat("CHECKED IN", "checkedValue", "#60a5fa");
    transitValue = makeStat("IN TRANSIT", "transitValue", "#fbbf24");
    arrivedValue = makeStat("ARRIVED", "arrivedValue", "#c4b5fd");
    deliveredValue = makeStat("DELIVERED", "deliveredValue", "#34d399");
    lostValue = makeStat("LOST / EXCEPTION", "lostValue", "#fb7185");
    outer->addLayout(stats);

    auto *progressPanel = panel();
    auto *pr = new QHBoxLayout(progressPanel);
    pr->setContentsMargins(14, 8, 14, 8);
    pr->addWidget(label("DELIVERY COMPLETION", "microTitle"));
    deliveryProgress = new QProgressBar;
    deliveryProgress->setObjectName("deliveryProgress");
    deliveryProgress->setRange(0, 100);
    pr->addWidget(deliveryProgress, 1);
    summaryLabel = label("0% delivered", "summaryLabel");
    pr->addWidget(summaryLabel);
    outer->addWidget(progressPanel);

    auto *tabs = new QTabWidget;
    tabs->setObjectName("mainTabs");
    QWidget *managementPage = new QWidget;
    auto *manage = new QVBoxLayout(managementPage);
    manage->setContentsMargins(0, 12, 0, 0);
    manage->setSpacing(12);

    auto *searchPanel = panel();
    auto *searchPanelLayout = new QVBoxLayout(searchPanel);
    searchPanelLayout->setContentsMargins(12, 10, 12, 10);
    searchPanelLayout->setSpacing(8);
    auto *searchRow = new QHBoxLayout;
    auto *filterRow = new QHBoxLayout;
    auto *quickAdd = new QPushButton("+  ADD BAGGAGE");
    quickAdd->setObjectName("primaryButton");
    quickAdd->setMinimumWidth(150);
    searchInput = new QLineEdit;
    searchInput->setPlaceholderText("Search ID, passenger, flight, destination...");
    searchInput->setClearButtonEnabled(true);
    auto *search = new QPushButton("Search");
    search->setObjectName("primaryButton");
    auto *reset = new QPushButton("Reset filters");
    reset->setObjectName("secondaryButton");
    statusFilter = new QComboBox;
    statusFilter->addItem("All statuses");
    statusFilter->addItems(STATUSES);
    statusFilter->setMinimumWidth(150);
    statusFilter->setMaximumWidth(190);
    airlineFilter = new QComboBox;
    airlineFilter->addItem("All airlines");
    airlineFilter->addItems(AIRLINES);
    airlineFilter->setMinimumWidth(155);
    airlineFilter->setMaximumWidth(190);
    searchRow->addWidget(quickAdd);
    searchRow->addWidget(searchInput, 1);
    searchRow->addWidget(search);
    searchRow->addWidget(reset);
    filterRow->addWidget(label("FILTER BY STATUS", "filterCaption"));
    filterRow->addWidget(statusFilter);
    filterRow->addSpacing(12);
    filterRow->addWidget(label("FILTER BY AIRLINE", "filterCaption"));
    filterRow->addWidget(airlineFilter);
    filterRow->addStretch();
    searchPanelLayout->addLayout(searchRow);
    searchPanelLayout->addLayout(filterRow);
    manage->addWidget(searchPanel);

    auto *actions = new QHBoxLayout;
    auto *add = new QPushButton("+  Add baggage"); add->setObjectName("primaryButton");
    auto *update = new QPushButton("Update status"); update->setObjectName("secondaryButton");
    auto *edit = new QPushButton("Edit details"); edit->setObjectName("secondaryButton");
    auto *details = new QPushButton("View details"); details->setObjectName("secondaryButton");
    auto *del = new QPushButton("Delete"); del->setObjectName("dangerButton");
    auto *exportBtn = new QPushButton("Export CSV"); exportBtn->setObjectName("secondaryButton");
    auto *undoBtn = new QPushButton("↶ Undo last change"); undoBtn->setObjectName("secondaryButton");
    actions->addWidget(add); actions->addWidget(update); actions->addWidget(edit);
    actions->addWidget(details); actions->addWidget(del); actions->addWidget(undoBtn); actions->addStretch(); actions->addWidget(exportBtn);
    manage->addLayout(actions);

    auto *manifestHeading = new QHBoxLayout;
    manifestHeading->addWidget(label("BAGGAGE MANIFEST", "sectionTitle"));
    manifestHeading->addStretch();
    selectionLabel = label("Select a row to enable record actions", "hintLabel");
    manifestHeading->addWidget(selectionLabel);
    manage->addLayout(manifestHeading);

    table = new QTableWidget;
    table->setColumnCount(5);
    table->setHorizontalHeaderLabels({"BAGGAGE ID", "PASSENGER", "FLIGHT", "DESTINATION", "STATUS  ·  CLICK TO CHANGE"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->verticalHeader()->setVisible(false);
    table->setAlternatingRowColors(true);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setFocusPolicy(Qt::NoFocus);
    table->setShowGrid(false);
    table->setWordWrap(false);
    table->verticalHeader()->setDefaultSectionSize(48);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setShowGrid(false);
    table->setSortingEnabled(false);
    table->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    table->verticalScrollBar()->setSingleStep(18);
    table->verticalScrollBar()->setPageStep(120);
    table->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    table->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    manage->addWidget(table, 1);

    QWidget *analyticsPage = new QWidget;
    auto *analytics = new QVBoxLayout(analyticsPage);
    analytics->setContentsMargins(0, 12, 0, 0);
    auto *chartRow = new QHBoxLayout;
    auto *piePanel = panel();
    piePanel->setMinimumHeight(350);
    auto *pieLayout = new QVBoxLayout(piePanel);
    pieLayout->setContentsMargins(14, 14, 14, 14);
    pieLayout->setSpacing(8);
    auto *pie = new AnalyticsChart(&records, AnalyticsChart::StatusPie);
    pie->setMinimumHeight(310);
    pieLayout->addWidget(pie);
    auto *barPanel = panel();
    barPanel->setMinimumHeight(350);
    auto *barLayout = new QVBoxLayout(barPanel);
    barLayout->setContentsMargins(14, 14, 14, 14);
    barLayout->setSpacing(8);
    auto *bars = new AnalyticsChart(&records, AnalyticsChart::AirlineBars);
    bars->setMinimumHeight(310);
    barLayout->addWidget(bars);
    chartRow->setSpacing(12);
    chartRow->addWidget(piePanel, 1);
    chartRow->addWidget(barPanel, 1);
chartRow->setSizeConstraint(QLayout::SetMinimumSize);
    analytics->addLayout(chartRow, 0);
    auto *activityPanel = panel();
    auto *activityLayout = new QVBoxLayout(activityPanel);
    activityLayout->setContentsMargins(14, 12, 14, 12);
    activityLayout->addWidget(label("RECENT ACTIVITY", "sectionTitle"));
    activityList = new QListWidget;
    activityList->setObjectName("activityList");
    activityList->setMinimumHeight(140);
    activityList->setMaximumHeight(220);
    activityList->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    activityList->verticalScrollBar()->setSingleStep(18);
    activityLayout->addWidget(activityList);
    activityPanel->setMinimumHeight(220);
    analytics->addWidget(activityPanel, 0);
    analytics->addWidget(label("Charts reflect the complete dataset and refresh after each successful change.", "hintLabel"));
    analytics->addStretch(1);
    chartsWidget = analyticsPage;

    auto *analyticsScroll = new QScrollArea;
    analyticsScroll->setObjectName("analyticsScroll");
    analyticsScroll->setWidgetResizable(true);
    analyticsScroll->setFrameShape(QFrame::NoFrame);
    analyticsScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    analyticsScroll->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    analyticsScroll->setWidget(analyticsPage);

    if (tabsLayout) {
        tabs->addTab(managementPage, "  Baggage Management  ");
        tabs->addTab(analyticsScroll, "  Analytics  ");
        outer->addWidget(tabs, 1);
    } else {
        auto *splitCharts = new QHBoxLayout;
        splitCharts->addWidget(piePanel, 1);
        splitCharts->addWidget(barPanel, 1);
        auto *content = new QWidget;
        auto *contentLayout = new QVBoxLayout(content);
        contentLayout->setContentsMargins(0, 0, 0, 0);
        contentLayout->setSpacing(12);
        contentLayout->addLayout(splitCharts, 1);
        contentLayout->addWidget(label("BAGGAGE MANIFEST", "sectionTitle"));

        // Detach these from the unused tab page before placing them on the one-screen layout.
        manage->removeWidget(searchPanel);
        manage->removeItem(actions);
        manage->removeWidget(table);
        searchPanel->setParent(content);
        table->setParent(content);
        contentLayout->addWidget(searchPanel);
        contentLayout->addLayout(actions);
        contentLayout->addWidget(table, 2);
        outer->addWidget(content, 1);
        delete managementPage;
    }

    setCentralWidget(central);
    statusBar()->showMessage("Ready • Local data store");

    connect(add, &QPushButton::clicked, this, &MainWindow::addBaggage);
    connect(quickAdd, &QPushButton::clicked, this, &MainWindow::addBaggage);
    connect(search, &QPushButton::clicked, this, &MainWindow::searchBaggage);
    connect(searchInput, &QLineEdit::returnPressed, this, &MainWindow::searchBaggage);
    connect(searchInput, &QLineEdit::textChanged, this, [this] { refreshTable(); });
    connect(statusFilter, &QComboBox::currentTextChanged, this, [this] { refreshTable(); });
    connect(airlineFilter, &QComboBox::currentTextChanged, this, [this] { refreshTable(); });
    connect(reset, &QPushButton::clicked, this, [this] {
        searchInput->clear(); statusFilter->setCurrentIndex(0); airlineFilter->setCurrentIndex(0);
        refreshTable();
    });
    connect(update, &QPushButton::clicked, this, &MainWindow::updateStatus);
    connect(edit, &QPushButton::clicked, this, &MainWindow::editDetails);
    connect(details, &QPushButton::clicked, this, &MainWindow::showRecordDetails);
    connect(del, &QPushButton::clicked, this, &MainWindow::deleteBaggage);
    connect(exportBtn, &QPushButton::clicked, this, &MainWindow::exportCsv);
    connect(undoBtn, &QPushButton::clicked, this, &MainWindow::undoLastAction);
    connect(table, &QTableWidget::itemSelectionChanged, this, [this] {
        selectionLabel->setText(selectedId().isEmpty() ? "Select a row to enable record actions"
                                                       : "Selected: " + selectedId());
    });
    connect(table, &QTableWidget::cellDoubleClicked, this, [this](int, int) { showRecordDetails(); });

    // Keep direct pointers to custom chart widgets; no QObject introspection needed.
    pieChartWidget = pie;
    barChartWidget = bars;
    pie->setCategoryCallback([this](const QString &status) {
        statusFilter->setCurrentText(status);
        if (tabsLayout) {
            if (auto *tabWidget = findChild<QTabWidget*>("mainTabs")) tabWidget->setCurrentIndex(0);
        }
    });
    bars->setCategoryCallback([this](const QString &airline) {
        airlineFilter->setCurrentText(airline);
        if (tabsLayout) {
            if (auto *tabWidget = findChild<QTabWidget*>("mainTabs")) tabWidget->setCurrentIndex(0);
        }
    });
}

void MainWindow::applyStyle() {
    setStyleSheet(R"(
      QMainWindow, QWidget { background:#0a1020; color:#e8eef8; font-family:"Segoe UI"; font-size:10pt; }
      QLabel#brandMark { color:#f0fbff; background:#12385a; border:1px solid #38bdf8; border-radius:12px; font-size:23pt; font-weight:900; }
      QLabel#brandName { color:#eaf6ff; font-size:11pt; font-weight:900; letter-spacing:1px; background:transparent; }
      QLabel#brandCaption { color:#62c6f5; font-size:7pt; font-weight:800; letter-spacing:1px; background:transparent; }
      QLabel#eyebrow { color:#62c6f5; font-size:9pt; font-weight:800; }
      QLabel#pageTitle { color:#f8fbff; font-size:25pt; font-weight:800; }
      QLabel#subtitle { color:#8fa4c2; font-size:10pt; }
      QLabel#livePill { color:#72f0bd; background:#102c2b; border:1px solid #1d5b51; border-radius:12px; padding:8px 12px; font-weight:800; }
      QLabel#clockLabel { color:#7e93b2; font-size:9pt; padding:5px; }
      QFrame#statCard, QFrame#panel { background:#111c30; border:1px solid #243653; border-radius:12px; }
      QLabel#statCaption, QLabel#microTitle, QLabel#filterCaption { color:#91a5c3; font-size:8pt; font-weight:800; background:transparent; }
      QLabel#totalValue, QLabel#checkedValue, QLabel#transitValue, QLabel#arrivedValue, QLabel#deliveredValue, QLabel#lostValue { font-size:22pt; font-weight:900; background:transparent; }
      QLabel#summaryLabel { color:#69e6b3; font-weight:800; background:transparent; }
      QLabel#sectionTitle { color:#d9e5f6; font-size:11pt; font-weight:900; letter-spacing:1px; background:transparent; }
      QLabel#hintLabel { color:#8297b5; font-size:9pt; background:transparent; }
      QLineEdit { background:#0b1527; color:#e8eef8; border:1px solid #2b3d5b; border-radius:8px; padding:10px 12px; min-height:22px; }
      QComboBox { background:#13233a; color:#e8eef8; border:1px solid #355274; border-radius:8px; padding:7px 24px 7px 10px; min-height:22px; }
      QTableWidget QComboBox { background:#142844; border:1px solid #31557c; border-radius:7px; padding:4px 22px 4px 8px; min-height:24px; font-weight:700; }
      QComboBox:hover { border:1px solid #38bdf8; background:#192d49; }
      QComboBox::drop-down { subcontrol-origin:padding; subcontrol-position:top right; width:28px; border-left:1px solid #355274; border-top-right-radius:8px; border-bottom-right-radius:8px; background:#1b3454; }
      QComboBox::down-arrow { width:9px; height:9px; }
      QComboBox QAbstractItemView { background:#13233a; color:#e8eef8; selection-background-color:#2563eb; selection-color:white; border:1px solid #355274; outline:0; padding:4px; }
      QLineEdit:focus, QComboBox:focus { border:1px solid #38bdf8; }
      QPushButton { border-radius:8px; padding:9px 13px; min-height:20px; font-weight:800; }
      QPushButton#primaryButton { background:#087fce; color:white; border:1px solid #38bdf8; }
      QPushButton#primaryButton:hover { background:#3b82f6; }
      QPushButton#secondaryButton { background:#203653; color:#f1f7ff; border:1px solid #4b6688; }
      QPushButton#secondaryButton:hover { background:#263d5e; }
      QPushButton#dangerButton { background:#a61b3b; color:#fff1f2; border:1px solid #fb7185; }
      QTableWidget { background:#0e192b; alternate-background-color:#111f34; color:#e8eef8; border:1px solid #253854; border-radius:10px; selection-background-color:#193e70; selection-color:white; outline:0; }
      QTableWidget::item { padding:8px 10px; border-bottom:1px solid #1b2b42; }
      QTableWidget::item:selected { background:#193e70; color:#ffffff; border-left:3px solid #38bdf8; }
      QTableWidget QScrollBar:vertical, QListWidget QScrollBar:vertical, QScrollArea QScrollBar:vertical { background:#0b1527; width:12px; margin:3px; border-radius:6px; }
      QTableWidget QScrollBar::handle:vertical, QListWidget QScrollBar::handle:vertical, QScrollArea QScrollBar::handle:vertical { background:#365477; min-height:28px; border-radius:6px; }
      QTableWidget QScrollBar::handle:vertical:hover, QListWidget QScrollBar::handle:vertical:hover, QScrollArea QScrollBar::handle:vertical:hover { background:#38bdf8; }
      QTableWidget QScrollBar::add-line:vertical, QTableWidget QScrollBar::sub-line:vertical, QListWidget QScrollBar::add-line:vertical, QListWidget QScrollBar::sub-line:vertical, QScrollArea QScrollBar::add-line:vertical, QScrollArea QScrollBar::sub-line:vertical { height:0; }
      QScrollArea { background:transparent; border:none; }
      QTableWidget::item:selected { background:#193e70; color:#ffffff; border-left:3px solid #38bdf8; }
      QTableWidget::item:hover { background:#172b46; }
      QListWidget#activityList { background:#0c1729; color:#cbd8eb; border:1px solid #243653; border-radius:8px; padding:6px; outline:0; }
      QListWidget#activityList::item { padding:7px; border-bottom:1px solid #1b2b42; }
      QListWidget#activityList::item:selected { background:#193e70; }
      QHeaderView::section { background:#182943; color:#bdd0ea; border:none; border-bottom:1px solid #2c4362; padding:12px 9px; font-size:9pt; font-weight:900; }
      QProgressBar { background:#111c30; color:#e8eef8; border:1px solid #2b3d5b; border-radius:7px; text-align:center; min-height:17px; font-weight:800; }
      QProgressBar::chunk { background:#10b981; border-radius:6px; }
      QTabWidget::pane { border:1px solid #243653; border-radius:10px; top:-1px; }
      QTabBar::tab { background:#111c30; color:#91a5c3; padding:12px 22px; margin-right:5px; border:1px solid #243653; border-top-left-radius:8px; border-top-right-radius:8px; }
      QTabBar::tab:selected { background:#1c3150; color:#f8fbff; border-bottom:2px solid #38bdf8; }
      QStatusBar { background:#0a1020; color:#7f94b2; border-top:1px solid #1b2b42; }

      QLabel#dialogEyebrow { color:#38bdf8; font-size:9pt; font-weight:900; background:transparent; }
      QLabel#dialogTitle { color:#f8fbff; font-size:17pt; font-weight:900; background:transparent; }
      QLabel#previewLabel { color:#6ee7b7; background:#102b2b; border:1px solid #1d5b51; border-radius:6px; padding:7px; }
      QDialog { background:#0d1728; color:#e8eef8; }
      QDialog QLabel { background:transparent; }
      QDialogButtonBox QPushButton { min-width:90px; }

    )");
}

int MainWindow::findRecord(const QString &id) const {
    const QString key = id.trimmed().toUpper();
    auto it = recordIndex.constFind(key);
    if (it != recordIndex.cend()) return it.value();
    // Safe fallback while loading legacy files or before the first index rebuild.
    for (int i = 0; i < records.size(); ++i)
        if (records.at(i).baggageID.compare(id.trimmed(), Qt::CaseInsensitive) == 0) return i;
    return -1;
}
void MainWindow::rebuildIndex() {
    recordIndex.clear();
    for (int i = 0; i < records.size(); ++i) recordIndex.insert(records.at(i).baggageID.toUpper(), i);
}
void MainWindow::pushUndo() {
    undoStack.push(records);
    while (undoStack.size() > 20) undoStack.removeAt(0);
}
void MainWindow::recordActivity(const QString &message) {
    const QString entry = QDateTime::currentDateTime().toString("hh:mm:ss") + "  ·  " + message;
    activityQueue.enqueue(entry);
    while (activityQueue.size() > 12) activityQueue.dequeue();
    if (activityList) {
        activityList->clear();
        for (int i = activityQueue.size() - 1; i >= 0; --i) activityList->addItem(activityQueue.at(i));
    }
}
void MainWindow::undoLastAction() {
    if (undoStack.isEmpty()) {
        QMessageBox::information(this, "Nothing to undo", "There are no recent changes to undo.");
        return;
    }
    records = undoStack.pop();
    rebuildIndex();
    saveToFile();
    recordActivity("Undo completed · previous baggage data restored");
    refreshAll();
}
void MainWindow::changeStatus(const QString &id, const QString &newStatus) {
    int ix = findRecord(id);
    if (ix < 0 || !STATUSES.contains(newStatus) || records[ix].status == newStatus) return;
    pushUndo();
    const QString oldStatus = records[ix].status;
    records[ix].status = newStatus;
    records[ix].updatedAt = QDateTime::currentDateTime().toString(Qt::ISODate);
    records[ix].statusHistory.append(records[ix].updatedAt + " — " + newStatus);
    saveToFile();
    recordActivity(id + " · " + oldStatus + " → " + newStatus);
    refreshAll();
}
QString MainWindow::selectedId() const {
    int row = table->currentRow();
    if (row < 0 || !table->item(row, 0)) return {};
    return table->item(row, 0)->text();
}
void MainWindow::refreshAll() {
    rebuildIndex();
    refreshTable(); refreshStats();
    if (pieChartWidget) pieChartWidget->update();
    if (barChartWidget) barChartWidget->update();
}
void MainWindow::refreshTable() {
    QString query = searchInput->text().trimmed(), status = statusFilter->currentText(), airline = airlineFilter->currentText();
    const QString previousSelection = selectedId();
    table->setSortingEnabled(false);
    table->setUpdatesEnabled(false);
    table->setRowCount(0);
    for (const Baggage &r : records) {
        if (status != "All statuses" && r.status != status) continue;
        if (airline != "All airlines" && r.airline != airline) continue;
        QString searchable = r.baggageID + " " + r.passengerName + " " + r.flightNumber + " " + r.destination + " " + r.airline;
        if (!query.isEmpty() && !searchable.contains(query, Qt::CaseInsensitive)) continue;
        int row = table->rowCount();
        table->insertRow(row);
        QStringList vals = {r.baggageID, r.passengerName, r.flightNumber, r.destination, r.status};
        for (int c = 0; c < 4; ++c) {
            auto *item = new QTableWidgetItem(vals[c]);
            item->setData(Qt::UserRole, r.baggageID);
            table->setItem(row, c, item);
        }
        auto *statusItem = new QTableWidgetItem(r.status);
        statusItem->setData(Qt::UserRole, r.baggageID);
        const int ix = STATUSES.indexOf(r.status);
        if (ix >= 0) statusItem->setForeground(QColor(STATUS_COLORS[ix]));
        table->setItem(row, 4, statusItem);
        auto *combo = new QComboBox;
        combo->addItems(STATUSES);
        combo->setCurrentText(r.status);
        combo->setProperty("baggageId", r.baggageID);
        combo->setToolTip("Change status for " + r.baggageID);
        combo->setMinimumHeight(34);
        connect(combo, &QComboBox::currentTextChanged, this, [this, combo](const QString &newStatus) {
            const QString id = combo->property("baggageId").toString();
            if (!id.isEmpty()) changeStatus(id, newStatus);
        });
        table->setCellWidget(row, 4, combo);
        if (r.baggageID == previousSelection) table->selectRow(row);
    }
    table->setUpdatesEnabled(true);
    table->viewport()->update();
    statusBar()->showMessage(QString("Showing %1 of %2 records · Local storage").arg(table->rowCount()).arg(records.size()));
    const QString id = selectedId();
    selectionLabel->setText(id.isEmpty() ? "Select a record · or enter an ID using an action" : "●  SELECTED  " + id);
}
void MainWindow::refreshStats() {
    int c[5]={};
    for(const auto &r:records){int i=STATUSES.indexOf(r.status);if(i>=0)c[i]++;}
    totalValue->setText(QString::number(records.size()));
    checkedValue->setText(QString::number(c[0])); transitValue->setText(QString::number(c[1]));
    arrivedValue->setText(QString::number(c[2])); deliveredValue->setText(QString::number(c[3]));
    lostValue->setText(QString::number(c[4]));
    int pct=records.isEmpty()?0:qRound(100.0*c[3]/records.size());
    deliveryProgress->setValue(pct); summaryLabel->setText(QString("%1% delivered").arg(pct));
}
void MainWindow::loadFromFile() {
    QFile f(dataPath()); if(!f.exists()) return;
    if(!f.open(QIODevice::ReadOnly)){QMessageBox::warning(this,"Load error","Could not read baggage_data.json.");return;}
    QJsonParseError e; auto doc=QJsonDocument::fromJson(f.readAll(),&e); f.close();
    if(e.error!=QJsonParseError::NoError||!doc.isArray()){QMessageBox::warning(this,"Load error","baggage_data.json is invalid; file was not overwritten.");return;}
    records.clear();
    for(const auto &v:doc.array()){
        if(!v.isObject())continue; auto o=v.toObject(); Baggage r;
        r.baggageID=o.value("baggageID").toString(); r.passengerName=o.value("passengerName").toString();
        r.flightNumber=o.value("flightNumber").toString(); r.destination=o.value("destination").toString();
        r.status=o.value("status").toString(); r.airline=o.value("airline").toString(); r.airlineCode=o.value("airlineCode").toString();
        r.createdAt=o.value("createdAt").toString(); r.updatedAt=o.value("updatedAt").toString();
        for (const auto &h : o.value("statusHistory").toArray()) r.statusHistory.append(h.toString());
        if (r.createdAt.isEmpty()) r.createdAt = QDateTime::currentDateTime().toString(Qt::ISODate);
        if (r.updatedAt.isEmpty()) r.updatedAt = r.createdAt;
        if (r.statusHistory.isEmpty()) r.statusHistory.append(r.createdAt + " — " + r.status);
        if(r.baggageID.isEmpty()||r.passengerName.isEmpty()||findRecord(r.baggageID)!=-1)continue;
        records.append(r);
        rebuildIndex();
    }
}
void MainWindow::saveToFile() {
    QJsonArray a;
    for(const auto&r:records){QJsonObject o;o["baggageID"]=r.baggageID;o["passengerName"]=r.passengerName;o["flightNumber"]=r.flightNumber;o["destination"]=r.destination;o["status"]=r.status;o["airline"]=r.airline;o["airlineCode"]=r.airlineCode;o["createdAt"]=r.createdAt;o["updatedAt"]=r.updatedAt;QJsonArray history;for(const auto &h:r.statusHistory)history.append(h);o["statusHistory"]=history;a.append(o);}
    const QString path = dataPath();
    if (QFile::exists(path)) {
        const QString backupPath = path + ".bak";
        if (QFile::exists(backupPath)) QFile::remove(backupPath);
        QFile::copy(path, backupPath);
    }
    QFile f(path); if(!f.open(QIODevice::WriteOnly|QIODevice::Truncate)){QMessageBox::warning(this,"Save error","Could not save baggage_data.json: "+f.errorString());return;}
    f.write(QJsonDocument(a).toJson(QJsonDocument::Indented)); f.close();
}

void MainWindow::addBaggage() {
    QDialog d(this); d.setWindowTitle("Register baggage"); d.setMinimumWidth(480);
    auto *layout=new QVBoxLayout(&d); layout->addWidget(label("NEW BAGGAGE RECORD","dialogEyebrow"));
    auto *form=new QFormLayout; form->setSpacing(12);
    auto *air=new QComboBox; air->addItems(AIRLINES);
    auto *id=new QLineEdit; id->setPlaceholderText("Baggage number must contain 5 or more digits"); id->setMaxLength(5);
    auto *name=new QLineEdit; name->setPlaceholderText("Passenger full name");
    auto *flight=new QLineEdit; flight->setPlaceholderText("Up to 3 digits"); flight->setMaxLength(3);
    auto *dest=new QLineEdit; dest->setPlaceholderText("Airport code or city");
    auto *status=new QComboBox; status->addItems(STATUSES);
    auto *idPreview=label("Generated ID: 6E•••••","previewLabel");
    auto *flightPreview=label("Generated flight: 6E-•••","previewLabel");
    auto updatePreview=[=]{int i=air->currentIndex();idPreview->setText("Generated ID: "+CODES[i]+(id->text().isEmpty()?"•••••":id->text()));flightPreview->setText("Generated flight: "+CODES[i]+"-"+(flight->text().isEmpty()?"•••":flight->text()));};
    QObject::connect(air,&QComboBox::currentTextChanged,&d,[=]{updatePreview();});
    QObject::connect(id,&QLineEdit::textChanged,&d,[=]{updatePreview();});
    QObject::connect(flight,&QLineEdit::textChanged,&d,[=]{updatePreview();});
    form->addRow("Airline",air); form->addRow("Baggage ID digits",id); form->addRow("",idPreview);
    form->addRow("Passenger",name); form->addRow("Flight digits",flight); form->addRow("",flightPreview);
    form->addRow("Destination",dest); form->addRow("Initial status",status); layout->addLayout(form);
    auto *buttons=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel); layout->addWidget(buttons);
    QObject::connect(buttons,&QDialogButtonBox::rejected,&d,&QDialog::reject);
    QObject::connect(buttons,&QDialogButtonBox::accepted,&d,[&]{
        if(!QRegularExpression("^[0-9]{1,5}$").match(id->text().trimmed()).hasMatch()){QMessageBox::warning(&d,"Invalid baggage ID","Enter between 1 and 5 digits.");return;}
        if(name->text().trimmed().isEmpty()||dest->text().trimmed().isEmpty()){QMessageBox::warning(&d,"Missing details","Passenger and destination are required.");return;}
        if(!QRegularExpression("^[0-9]{1,3}$").match(flight->text().trimmed()).hasMatch()){QMessageBox::warning(&d,"Invalid flight","Enter 1 to 3 flight digits.");return;}
        QString full=CODES[air->currentIndex()]+id->text().trimmed();
        if(findRecord(full)!=-1){QMessageBox::warning(&d,"Duplicate ID","This baggage ID already exists.");return;}
        d.accept();
    });
    updatePreview();
    if(d.exec()!=QDialog::Accepted)return;
    int i=air->currentIndex(); Baggage r;
    r.airline=air->currentText();r.airlineCode=CODES[i];r.baggageID=CODES[i]+id->text().trimmed();
    r.passengerName=name->text().trimmed();r.flightNumber=CODES[i]+"-"+flight->text().trimmed();
    r.destination=dest->text().trimmed();r.status=status->currentText();
    r.createdAt = QDateTime::currentDateTime().toString(Qt::ISODate);
    r.updatedAt = r.createdAt;
    r.statusHistory.append(r.createdAt + " — " + r.status);
    pushUndo(); records.append(r); saveToFile(); recordActivity("Baggage registered · " + r.baggageID); refreshAll();
}

void MainWindow::searchBaggage(){if(searchInput->text().trimmed().isEmpty()){QMessageBox::information(this,"Search","Enter a search term.");return;}refreshTable();if(table->rowCount())table->selectRow(0);else QMessageBox::information(this,"No matches","No records match your search.");}
void MainWindow::updateStatus() {
    QString id = selectedId();
    if (id.isEmpty()) {
        bool ok = false;
        id = QInputDialog::getText(this, "Choose baggage", "Enter the baggage ID to update:", QLineEdit::Normal, "", &ok).trimmed();
        if (!ok || id.isEmpty()) return;
    }
    int ix = findRecord(id);
    if (ix < 0) { QMessageBox::warning(this, "Baggage not found", "No baggage record exists with ID: " + id); return; }
    QDialog d(this); d.setWindowTitle("Update baggage status"); d.setMinimumWidth(400);
    auto *v = new QVBoxLayout(&d); v->addWidget(label("UPDATE STATUS", "dialogEyebrow")); v->addWidget(label("Baggage: " + id, "dialogTitle"));
    auto *combo = new QComboBox; combo->addItems(STATUSES); combo->setCurrentText(records[ix].status); v->addWidget(label("Choose the new status:")); v->addWidget(combo);
    auto *bb = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel); v->addWidget(bb);
    connect(bb, &QDialogButtonBox::rejected, &d, &QDialog::reject); connect(bb, &QDialogButtonBox::accepted, &d, &QDialog::accept);
    if (d.exec() != QDialog::Accepted) return;
    changeStatus(id, combo->currentText());
}
void MainWindow::editDetails() {
    QString id=selectedId();
    if(id.isEmpty()){
        bool ok=false;
        id=QInputDialog::getText(this,"Choose baggage","Enter the baggage ID to edit:",QLineEdit::Normal,"",&ok).trimmed();
        if(!ok||id.isEmpty())return;
    }
    int ix=findRecord(id);
    if(ix<0){QMessageBox::warning(this,"Baggage not found","No baggage record exists with ID: "+id);return;}
    Baggage &r=records[ix];
    QDialog d(this);d.setWindowTitle("Edit baggage details");d.setMinimumWidth(460);
    auto *v=new QVBoxLayout(&d);v->addWidget(label("EDIT RECORD","dialogEyebrow"));v->addWidget(label("Baggage ID: "+id,"dialogTitle"));
    auto *form=new QFormLayout;auto *name=new QLineEdit(r.passengerName);auto *dest=new QLineEdit(r.destination);
    auto *flight=new QLineEdit(r.flightNumber.section('-',1));flight->setMaxLength(3);flight->setPlaceholderText("Up to 3 digits, e.g. 998");form->addRow("Passenger",name);form->addRow("Destination",dest);form->addRow("Flight digits",flight);v->addLayout(form);
    auto *bb=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel);v->addWidget(bb);
    connect(bb,&QDialogButtonBox::rejected,&d,&QDialog::reject);
    connect(bb,&QDialogButtonBox::accepted,&d,[&]{if(name->text().trimmed().isEmpty()||dest->text().trimmed().isEmpty()||!QRegularExpression("^[0-9]{1,3}$").match(flight->text().trimmed()).hasMatch()){QMessageBox::warning(&d,"Check details","Enter a passenger, destination, and 1–3 flight digits.");return;}d.accept();});
    if(d.exec()!=QDialog::Accepted)return;
    pushUndo(); r.passengerName=name->text().trimmed();r.destination=dest->text().trimmed();r.flightNumber=r.airlineCode+"-"+flight->text().trimmed();
    r.updatedAt = QDateTime::currentDateTime().toString(Qt::ISODate); saveToFile(); recordActivity("Details edited · " + id); refreshAll();
}
void MainWindow::showRecordDetails() {
    QString id=selectedId();
    if(id.isEmpty()){
        bool ok=false;
        id=QInputDialog::getText(this,"Choose baggage","Enter the baggage ID to view:",QLineEdit::Normal,"",&ok).trimmed();
        if(!ok||id.isEmpty())return;
    }
    int ix=findRecord(id);
    if(ix<0){QMessageBox::warning(this,"Baggage not found","No baggage record exists with ID: "+id);return;}
    const auto&r=records[ix];
    QDialog d(this);d.setWindowTitle("Baggage record details");d.setMinimumWidth(440);
    auto *v=new QVBoxLayout(&d);v->addWidget(label("BAGGAGE RECORD","dialogEyebrow"));v->addWidget(label(r.baggageID,"dialogTitle"));
    auto *form=new QFormLayout;form->addRow("Passenger",label(r.passengerName));form->addRow("Airline",label(r.airline));form->addRow("Flight",label(r.flightNumber));form->addRow("Destination",label(r.destination));form->addRow("Current status",label(r.status));form->addRow("Created",label(r.createdAt));form->addRow("Last updated",label(r.updatedAt));v->addLayout(form);
    v->addWidget(label("JOURNEY TIMELINE", "sectionTitle"));
    auto *timeline = new QListWidget;
    for (const auto &event : r.statusHistory) timeline->addItem(event);
    timeline->setMinimumHeight(100); timeline->setMaximumHeight(180); v->addWidget(timeline);
    auto *bb=new QDialogButtonBox(QDialogButtonBox::Close);v->addWidget(bb);connect(bb,&QDialogButtonBox::rejected,&d,&QDialog::reject);connect(bb,&QDialogButtonBox::accepted,&d,&QDialog::accept);d.exec();
}
void MainWindow::deleteBaggage() {
    QString id=selectedId();
    if(id.isEmpty()){
        bool ok=false;
        id=QInputDialog::getText(this,"Choose baggage","Enter the baggage ID to delete:",QLineEdit::Normal,"",&ok).trimmed();
        if(!ok||id.isEmpty())return;
    }
    int ix=findRecord(id);
    if(ix<0){QMessageBox::warning(this,"Baggage not found","No baggage record exists with ID: "+id);return;}
    auto ans=QMessageBox::question(this,"Confirm deletion","Delete baggage "+id+"?",QMessageBox::Yes|QMessageBox::No,QMessageBox::No);
    if(ans!=QMessageBox::Yes)return;
    pushUndo(); records.removeAt(ix); rebuildIndex(); saveToFile(); recordActivity("Baggage deleted · " + id); refreshAll();
}
void MainWindow::exportCsv() {
    QString path=QFileDialog::getSaveFileName(this,"Export manifest","baggage_manifest.csv","CSV files (*.csv)");if(path.isEmpty())return;
    QFile f(path);if(!f.open(QIODevice::WriteOnly|QIODevice::Text)){QMessageBox::warning(this,"Export error","Could not create CSV file.");return;}
    QTextStream out(&f);out<<"Baggage ID,Passenger Name,Flight Number,Destination,Status,Airline,Airline Code\n";
    for(const auto&r:records){QStringList fields={r.baggageID,r.passengerName,r.flightNumber,r.destination,r.status,r.airline,r.airlineCode};for(auto &x:fields){x.replace("\"","\"\"");x="\""+x+"\"";}out<<fields.join(",")<<"\n";}
    f.close();QMessageBox::information(this,"Export complete",QString("Exported %1 records.").arg(records.size()));
}
