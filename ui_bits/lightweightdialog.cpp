#include "lightweightdialog.h"
#include "ui_lightweightdialog.h"
#include <vector>
#include <iostream>
lightweightdialog::lightweightdialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::lightweightdialog),
    parent_(parent),
    slider_layout(new QHBoxLayout(this))
{
    ui->setupUi(this);
}

lightweightdialog::~lightweightdialog()
{
    delete ui;
}
void lightweightdialog::drawSliders(int nlights)
{
    for (int slider = 0; slider < sliders.size(); ++slider) {
        delete sliders[slider];
    }
    sliders.clear();
    weights_ = std::vector<float>(nlights);
    delete slider_layout;


    slider_layout = new QHBoxLayout(this);
    nlights_ = nlights;
    int initial_slider_value = (1/(float)nlights_) * 100;
    std::vector<float> default_weights;
    for (auto i = 0; i < nlights; ++i) {
        default_weights.push_back(1/(float)nlights_);
    }

    for (int light = 0; light < nlights_; ++light) {
        QSlider *slider = new QSlider();
        slider->setMinimum(0);
        slider->setMaximum(100);
        slider->setValue(initial_slider_value);
        slider->setObjectName(QString::number(light));
        slider->setValue(default_weights[light] * 100);
        sliders.push_back(slider);
        slider_layout->addWidget(slider);
        slider_layout->addSpacing(20);
        connect(slider, SIGNAL(sliderReleased()), this, SLOT(handleSliderRelease()));
       // connect(slider, SIGNAL(sliderMoved(int)), this, SLOT(handleSliderChange(int)));
    }
    this->adjustSize();
}

void lightweightdialog::handleSliderRelease()
{

    float slider_sum = 0;
    for (auto light = 0; light < nlights_; ++light) {
        slider_sum += sliders[light]->value();
    }
    for (auto light = 0; light < nlights_; ++light) {
        weights_[light] = (float)sliders[light]->value() / (float)slider_sum;
    }
    emit weightsChanged(weights_);
}

void lightweightdialog::setWeights(std::vector<float> weights)
{
    weights_ = weights;
}

