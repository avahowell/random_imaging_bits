#ifndef LIGHTWEIGHTDIALOG_H
#define LIGHTWEIGHTDIALOG_H

#include <QDialog>
#include <QSlider>
#include <QHBoxLayout>
#include <vector>

namespace Ui {
class lightweightdialog;
}

class lightweightdialog : public QDialog
{
    Q_OBJECT

public:
    explicit lightweightdialog(QWidget *parent = 0);
    ~lightweightdialog();
    void startWeight(int nlights);
    void drawSliders(int nlights);
    void setWeights(std::vector<float> weights);

private:
    QObject *parent_;
    QHBoxLayout *slider_layout;
    std::vector< QSlider *> sliders;
    Ui::lightweightdialog *ui;
    std::vector<float> weights_;
    int nlights_;
private slots:
    //void handleSliderChange(int val);
    void handleSliderRelease();
signals:
    void weightsChanged(std::vector<float>);
};

#endif // LIGHTWEIGHTDIALOG_H
