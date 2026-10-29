#ifndef PAGEBAR_H
#define PAGEBAR_H

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QWidget>

#include <functional>
#include <utility>

// "Rows per page [25|50|100]  ....  1-50 of 312  [< Prev] [Next >]" bar shared
// by the list windows (Customers, Invoices, Payments, Items).
//
// Usage in a window:
//   ctor:    m_pageBar = new PageBar([this]() { reload(); }, this);
//            ui->verticalLayout->addWidget(m_pageBar);
//   reload:  m_pageBar->setTotal(dao.count...(...));          // clamps the page
//            rows = dao.search...Page(..., m_pageBar->limit(), m_pageBar->offset());
//   search text / filter changed:  m_pageBar->resetPage(); reload();
//
// The callback runs when the user clicks Prev/Next or changes the page size.
// Header-only and deliberately without Q_OBJECT, so nothing needs moc or an
// entry in the project file.
class PageBar : public QWidget
{
public:
    explicit PageBar(std::function<void()> onChange, QWidget* parent = nullptr)
        : QWidget(parent)
        , m_onChange(std::move(onChange))
    {
        setObjectName(QStringLiteral("PageBar"));

        auto* row = new QHBoxLayout(this);
        row->setContentsMargins(0, 4, 0, 0);

        row->addWidget(new QLabel(QStringLiteral("Rows per page:"), this));

        m_sizeBox = new QComboBox(this);
        m_sizeBox->setObjectName(QStringLiteral("PageSize_ComboBox"));
        for (const int size : {25, 50, 100})
            m_sizeBox->addItem(QString::number(size), size);
        m_sizeBox->setCurrentIndex(m_sizeBox->findData(kDefaultPageSize));
        row->addWidget(m_sizeBox);

        row->addStretch(1);

        m_info = new QLabel(this);
        m_info->setObjectName(QStringLiteral("PageInfo_Label"));
        row->addWidget(m_info);

        m_prev = new QPushButton(QStringLiteral("< Prev"), this);
        m_prev->setObjectName(QStringLiteral("PagePrev_Button"));
        row->addWidget(m_prev);

        m_next = new QPushButton(QStringLiteral("Next >"), this);
        m_next->setObjectName(QStringLiteral("PageNext_Button"));
        row->addWidget(m_next);

        updateControls();

        connect(m_sizeBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this]() {
            m_page = 0;
            changed();
        });
        connect(m_prev, &QPushButton::clicked, this, [this]() {
            if (m_page > 0) {
                --m_page;
                changed();
            }
        });
        connect(m_next, &QPushButton::clicked, this, [this]() {
            if (m_page + 1 < pageCount()) {
                ++m_page;
                changed();
            }
        });
    }

    // Rows to fetch / skip for the current page.
    int limit() const { return m_sizeBox->currentData().toInt(); }
    int offset() const { return m_page * limit(); }

    // Call when the search text or a filter changes: back to the first page.
    void resetPage() { m_page = 0; }

    // Tell the bar how many rows match the current search. If the current page
    // no longer exists (e.g. the last row on it was deleted) it moves back to
    // the last page, so read offset() AFTER calling this.
    void setTotal(int total)
    {
        m_total = qMax(0, total);
        const int last = pageCount() - 1;
        if (m_page > last)
            m_page = last;
        updateControls();
    }

private:
    static constexpr int kDefaultPageSize = 50;

    int pageCount() const
    {
        const int size = limit();
        return m_total == 0 ? 1 : (m_total + size - 1) / size;
    }

    void updateControls()
    {
        if (m_total == 0) {
            m_info->setText(QStringLiteral("No results"));
        } else {
            const int first = offset() + 1;
            const int last = qMin(offset() + limit(), m_total);
            m_info->setText(QStringLiteral("%1-%2 of %3").arg(first).arg(last).arg(m_total));
        }
        m_prev->setEnabled(m_page > 0);
        m_next->setEnabled(m_page + 1 < pageCount());
    }

    void changed()
    {
        if (m_onChange)
            m_onChange();
        else
            updateControls();
    }

    std::function<void()> m_onChange;
    QComboBox* m_sizeBox{nullptr};
    QLabel* m_info{nullptr};
    QPushButton* m_prev{nullptr};
    QPushButton* m_next{nullptr};
    int m_page{0};     // 0-based
    int m_total{0};
};

#endif // PAGEBAR_H
