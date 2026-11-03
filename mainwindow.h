#ifndef MAINWINDOW_H
#define MAINWINDOW_H
#include <QMainWindow>
#include <QList>
#include <QString>
#include <QStringList>
#include <QHash>
#include <QStack>
#include <QQueue>

class QLineEdit;
class QLabel;
class QPushButton;
class QTableWidget;
class QComboBox;
class QProgressBar;
class QWidget;
class QListWidget;

struct Baggage {
    QString baggageID, passengerName, flightNumber, destination, status, airline, airlineCode;
    QString createdAt, updatedAt;
    QStringList statusHistory;
};

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
private:
    QList<Baggage> records;
    QLineEdit *searchInput = nullptr;
    QComboBox *statusFilter = nullptr;
    QComboBox *airlineFilter = nullptr;
    QTableWidget *table = nullptr;
    QLabel *totalValue = nullptr, *checkedValue = nullptr, *transitValue = nullptr;
    QLabel *arrivedValue = nullptr, *deliveredValue = nullptr, *lostValue = nullptr;
    QLabel *summaryLabel = nullptr, *clockLabel = nullptr, *selectionLabel = nullptr;
    QProgressBar *deliveryProgress = nullptr;
    QWidget *chartsWidget = nullptr;
    QWidget *pieChartWidget = nullptr;
    QWidget *barChartWidget = nullptr;
    QListWidget *activityList = nullptr;
    QHash<QString, int> recordIndex;
    QStack<QList<Baggage>> undoStack;
    QQueue<QString> activityQueue;
    bool tabsLayout = false;

    void buildUi();
    void applyStyle();
    void buildCharts(QWidget *parent);
    void refreshAll();
    void refreshTable();
    void refreshStats();
    void rebuildIndex();
    void pushUndo();
    void recordActivity(const QString &message);
    void undoLastAction();
    void changeStatus(const QString &id, const QString &newStatus);
    void loadFromFile();
    void saveToFile();
    int findRecord(const QString &id) const;
    QString selectedId() const;
    void addBaggage();
    void searchBaggage();
    void updateStatus();
    void editDetails();
    void deleteBaggage();
    void exportCsv();
    void showRecordDetails();
};
#endif