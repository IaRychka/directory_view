#include <QApplication>
#include <QFileSystemModel>
#include <QFileIconProvider>
#include <QScreen>
#include <QScroller>
#include <QTreeView>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QPushButton>
#include <QHeaderView>
#include <QDir>
#include <QFileInfo>
#include <QMessageBox>
#include <QSortFilterProxyModel>

class DirView : public QWidget
{
public:
    DirView(QWidget *parent = nullptr) : QWidget(parent)
    {
        QVBoxLayout *layout = new QVBoxLayout(this);

        filterLineEdit = new QLineEdit(this);
        connect(filterLineEdit, &QLineEdit::textChanged, this, &DirView::filterChanged);
        layout->addWidget(filterLineEdit);

        tree = new QTreeView(this);
        layout->addWidget(tree);

        model = new QFileSystemModel(this);
        model->setRootPath(QDir::homePath());
        model->setFilter(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Hidden);

        proxyModel = new QSortFilterProxyModel(this);
        proxyModel->setSourceModel(model);
        proxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
        proxyModel->setFilterKeyColumn(0); // Filter by the name column

        tree->setModel(proxyModel);
        tree->setRootIndex(proxyModel->mapFromSource(model->index(QDir::homePath())));

        // Demonstrating look and feel features
        tree->setAnimated(false);
        tree->setIndentation(20);
        tree->setSortingEnabled(true);
        const QSize availableSize = tree->screen()->availableGeometry().size();
        tree->resize(availableSize / 2);
        tree->setColumnWidth(0, tree->width() / 3);

        // Make it flickable on touchscreens
        QScroller::grabGesture(tree, QScroller::TouchGesture);

        // Add "Size" column
        model->setHeaderData(1, Qt::Horizontal, QObject::tr("Size"));
        model->setHeaderData(2, Qt::Horizontal, QObject::tr("Folder Size"));

        connect(tree, &QTreeView::clicked, this, &DirView::onItemClicked);
    }

private:
    void filterChanged(const QString &text)
    {
        proxyModel->setFilterWildcard(text);
    }

    void onItemClicked(const QModelIndex &index)
    {
        QModelIndex sourceIndex = proxyModel->mapToSource(index);
        if (model->isDir(sourceIndex)) {
            QFileInfo fileInfo = model->fileInfo(sourceIndex);
            QPushButton *button = new QPushButton(QObject::tr("Update"), this);
            connect(button, &QPushButton::clicked, [this, sourceIndex]() {
                QFileInfo fileInfo = model->fileInfo(sourceIndex);
                if (fileInfo.isDir()) {
                    quint64 size = dirSize(fileInfo.absoluteFilePath());
                    model->setData(sourceIndex.sibling(sourceIndex.row(), 2), QString::number(size) + " bytes", Qt::DisplayRole);
                }
            });
            model->setData(sourceIndex.sibling(sourceIndex.row(), 2), QVariant::fromValue(static_cast<QWidget*>(button)), Qt::UserRole);
        }
    }

    quint64 dirSize(const QString &path)
    {
        quint64 size = 0;
        QDir dir(path);
        QFileInfoList list = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden);
        foreach (QFileInfo info, list) {
            if (info.isDir()) {
                size += dirSize(info.absoluteFilePath());
            } else {
                size += info.size();
            }
        }
        return size;
    }

    QLineEdit *filterLineEdit;
    QTreeView *tree;
    QFileSystemModel *model;
    QSortFilterProxyModel *proxyModel;
};

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QCoreApplication::setApplicationVersion(QT_VERSION_STR);
    QCommandLineParser parser;
    parser.setApplicationDescription("Qt Dir View Example");
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption dontUseCustomDirectoryIconsOption("c", "Set QFileSystemModel::DontUseCustomDirectoryIcons");
    parser.addOption(dontUseCustomDirectoryIconsOption);
    QCommandLineOption dontWatchOption("w", "Set QFileSystemModel::DontWatch");
    parser.addOption(dontWatchOption);
    parser.process(app);

    DirView dirView;
    dirView.setWindowTitle(QObject::tr("Dir View"));
    dirView.resize(800, 600);
    dirView.show();

    return app.exec();
}
