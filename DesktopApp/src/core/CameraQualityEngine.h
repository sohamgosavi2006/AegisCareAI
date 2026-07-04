#ifndef CAMERAQUALITYENGINE_H
#define CAMERAQUALITYENGINE_H

#include <QObject>
#include <QImage>
#include <QRect>

#ifdef AEGISCARE_HAS_OPENCV
#include <opencv2/opencv.hpp>
#endif

class CameraQualityEngine : public QObject {
    Q_OBJECT
public:
    static CameraQualityEngine& instance();

    struct QualityStats {
        double lighting = 0.0;     // 0 to 100 (ideal: 50-80)
        double clarity = 0.0;      // 0 to 100 (blur index)
        double stability = 0.0;    // 0 to 100 (100 = completely still)
        double framing = 0.0;      // 0 to 100 (centered face/organ)
        double totalScore = 0.0;   // average score
        bool faceDetected = false;
        QRect faceLocation;
    };

    void init();
    QualityStats analyzeFrame(const QImage& qimg);

#ifdef AEGISCARE_HAS_OPENCV
    static cv::Mat qimageToMat(const QImage& qimg);
    static QImage matToQImage(const cv::Mat& mat);
#endif

signals:
    void qualityAnalyzed(const CameraQualityEngine::QualityStats& stats);

private:
    CameraQualityEngine(QObject* parent = nullptr);
    ~CameraQualityEngine();
    CameraQualityEngine(const CameraQualityEngine&) = delete;
    CameraQualityEngine& operator=(const CameraQualityEngine&) = delete;

#ifdef AEGISCARE_HAS_OPENCV
    cv::CascadeClassifier m_faceCascade;
    cv::Mat m_prevFrame;
#else
    QImage m_prevFrame;
#endif
    bool m_cascadeLoaded = false;

    void loadCascade();
};

#endif // CAMERAQUALITYENGINE_H
