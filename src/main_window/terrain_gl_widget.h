#pragma once

#include "../map_io/terrain_model.h"

#include <QOpenGLBuffer>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QPoint>
#include <QVector>
#include <QVector3D>

class TerrainGlWidget : public QOpenGLWidget, protected QOpenGLFunctions {
	Q_OBJECT

public:
	explicit TerrainGlWidget(QWidget* parent = nullptr);
	~TerrainGlWidget() override;

	void setTerrainModel(const TerrainModel& model);

protected:
	void initializeGL() override;
	void resizeGL(int w, int h) override;
	void paintGL() override;
	void mousePressEvent(QMouseEvent* event) override;
	void mouseMoveEvent(QMouseEvent* event) override;
	void mouseReleaseEvent(QMouseEvent* event) override;
	void wheelEvent(QWheelEvent* event) override;

private:
	void rebuildVertices();
	void resetCameraToFitMap();
	static QVector3D colorForKey(const QString& key);

	TerrainModel model_;
	QVector<float> vertices_;
	bool dirty_ = false;

	QOpenGLShaderProgram program_;
	QOpenGLBuffer vbo_ {QOpenGLBuffer::VertexBuffer};
	QOpenGLVertexArrayObject vao_;

	QPoint lastMousePos_;
	bool rotating_ = false;
	bool panning_ = false;
	float yawDeg_ = 90.0f;
	float pitchDeg_ = 50.0f;
	float distance_ = 1800.0f;
	QVector3D target_ = QVector3D(0.0f, 0.0f, 0.0f);
};
