#include "terrain_gl_widget.h"

#include <QMatrix4x4>
#include <QMouseEvent>
#include <QVector3D>
#include <QWheelEvent>
#include <QtMath>
#include <algorithm>

namespace {
constexpr int kStrideFloats = 6; // pos.xyz + color.rgb
}

TerrainGlWidget::TerrainGlWidget(QWidget* parent) : QOpenGLWidget(parent) {
	setMinimumHeight(300);
}

TerrainGlWidget::~TerrainGlWidget() {
	makeCurrent();
	vao_.destroy();
	vbo_.destroy();
	doneCurrent();
}

void TerrainGlWidget::setTerrainModel(const TerrainModel& model) {
	model_ = model;
	resetCameraToFitMap();
	dirty_ = true;
	update();
}

void TerrainGlWidget::initializeGL() {
	initializeOpenGLFunctions();
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
	glClearColor(0.08f, 0.08f, 0.10f, 1.0f);

	program_.addShaderFromSourceCode(QOpenGLShader::Vertex, R"(
		#version 330 core
		layout(location = 0) in vec3 aPos;
		layout(location = 1) in vec3 aColor;
		uniform mat4 uMVP;
		out vec3 vColor;
		void main() {
			gl_Position = uMVP * vec4(aPos, 1.0);
			vColor = aColor;
		}
	)");
	program_.addShaderFromSourceCode(QOpenGLShader::Fragment, R"(
		#version 330 core
		in vec3 vColor;
		out vec4 FragColor;
		void main() {
			FragColor = vec4(vColor, 1.0);
		}
	)");
	program_.link();

	vao_.create();
	vbo_.create();
}

void TerrainGlWidget::resizeGL(int w, int h) {
	glViewport(0, 0, w, h);
}

QVector3D TerrainGlWidget::colorForKey(const QString& key) {
	const uint h = qHash(key);
	const float r = 0.30f + static_cast<float>(h & 0x7F) / 255.0f;
	const float g = 0.30f + static_cast<float>((h >> 7) & 0x7F) / 255.0f;
	const float b = 0.30f + static_cast<float>((h >> 14) & 0x7F) / 255.0f;
	return QVector3D(std::min(r, 0.95f), std::min(g, 0.95f), std::min(b, 0.95f));
}

void TerrainGlWidget::rebuildVertices() {
	vertices_.clear();
	if (!model_.valid || model_.width < 2 || model_.height < 2 || model_.tiles.isEmpty()) {
		return;
	}

	const int tileWidth = model_.width - 1;
	const int tileHeight = model_.height - 1;
	const float zScale = 48.0f;
	vertices_.reserve(tileWidth * tileHeight * 6 * kStrideFloats);

	auto cornerHeight = [&](int x, int y) -> float {
		const int idx = y * model_.width + x;
		if (idx < 0 || idx >= model_.corners.size()) return 0.0f;
		return model_.corners[idx].height * zScale;
	};

	auto emitVertex = [&](float x, float y, float z, const QVector3D& c) {
		vertices_.push_back(x);
		vertices_.push_back(y);
		vertices_.push_back(z);
		vertices_.push_back(c.x());
		vertices_.push_back(c.y());
		vertices_.push_back(c.z());
	};

	for (const TerrainMaterialBatch& batch : model_.drawBatches.ground) {
		const QVector3D color = colorForKey(batch.texturePath);
		for (quint32 tileIndexU32 : batch.tileIndices) {
			const int tileIndex = static_cast<int>(tileIndexU32);
			if (tileIndex < 0 || tileIndex >= tileWidth * tileHeight) continue;

			const int tx = tileIndex % tileWidth;
			const int ty = tileIndex / tileWidth;

			const float x0 = static_cast<float>(model_.offset.x() + tx * 128.0);
			const float y0 = static_cast<float>(model_.offset.y() + ty * 128.0);
			const float x1 = x0 + 128.0f;
			const float y1 = y0 + 128.0f;

			const float z00 = cornerHeight(tx, ty);
			const float z10 = cornerHeight(tx + 1, ty);
			const float z01 = cornerHeight(tx, ty + 1);
			const float z11 = cornerHeight(tx + 1, ty + 1);

			emitVertex(x0, y0, z00, color);
			emitVertex(x1, y0, z10, color);
			emitVertex(x1, y1, z11, color);

			emitVertex(x0, y0, z00, color);
			emitVertex(x1, y1, z11, color);
			emitVertex(x0, y1, z01, color);
		}
	}
}

void TerrainGlWidget::resetCameraToFitMap() {
	if (!model_.valid || model_.width < 2 || model_.height < 2) {
		target_ = QVector3D(0.0f, 0.0f, 0.0f);
		distance_ = 1800.0f;
		yawDeg_ = 90.0f;
		pitchDeg_ = 50.0f;
		return;
	}

	const float mapWidth = std::max(1.0f, static_cast<float>(model_.width - 1) * 128.0f);
	const float mapHeight = std::max(1.0f, static_cast<float>(model_.height - 1) * 128.0f);
	target_ = QVector3D(
		static_cast<float>(model_.offset.x()) + mapWidth * 0.5f,
		static_cast<float>(model_.offset.y()) + mapHeight * 0.5f,
		0.0f);

	// YDWE-like startup: bird-eye view focused to roughly a 2048-world-unit area.
	constexpr float targetVisibleSpan = 2048.0f;
	constexpr float fovRad = 50.0f * static_cast<float>(M_PI) / 180.0f;
	const float spanDistance = targetVisibleSpan / (2.0f * std::tan(fovRad * 0.5f));
	distance_ = std::clamp(spanDistance, 900.0f, std::max(mapWidth, mapHeight) * 1.8f);
	yawDeg_ = 90.0f;
	pitchDeg_ = 68.0f;
}

void TerrainGlWidget::paintGL() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	if (dirty_) {
		rebuildVertices();
		vao_.bind();
		vbo_.bind();
		vbo_.allocate(vertices_.constData(), static_cast<int>(vertices_.size() * sizeof(float)));
		program_.bind();
		program_.enableAttributeArray(0);
		program_.setAttributeBuffer(0, GL_FLOAT, 0, 3, kStrideFloats * static_cast<int>(sizeof(float)));
		program_.enableAttributeArray(1);
		program_.setAttributeBuffer(1, GL_FLOAT, 3 * static_cast<int>(sizeof(float)), 3, kStrideFloats * static_cast<int>(sizeof(float)));
		program_.release();
		vbo_.release();
		vao_.release();
		dirty_ = false;
	}

	if (vertices_.isEmpty()) {
		return;
	}

	const float aspect = std::max(0.001f, static_cast<float>(width()) / std::max(1, height()));
	QMatrix4x4 proj;
	proj.perspective(50.0f, aspect, 1.0f, 1000000.0f);

	const float yawRad = qDegreesToRadians(yawDeg_);
	const float pitchRad = qDegreesToRadians(pitchDeg_);
	const QVector3D forward(
		std::cos(pitchRad) * std::cos(yawRad),
		std::cos(pitchRad) * std::sin(yawRad),
		std::sin(pitchRad));
	const QVector3D eye = target_ - forward * distance_;
	const QVector3D worldUp(0.0f, 0.0f, 1.0f);
	QMatrix4x4 view;
	view.lookAt(eye, target_, worldUp);

	const QMatrix4x4 mvp = proj * view;
	program_.bind();
	program_.setUniformValue("uMVP", mvp);
	vao_.bind();
	glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices_.size() / kStrideFloats));
	vao_.release();
	program_.release();
}

void TerrainGlWidget::mousePressEvent(QMouseEvent* event) {
	lastMousePos_ = event->pos();
	rotating_ = false;
	panning_ = false;
	if (event->button() == Qt::RightButton) {
		if (event->modifiers() & Qt::ControlModifier) rotating_ = true;
		else panning_ = true;
	}
	// Left mouse is reserved for object selection / paint tools.
	event->accept();
}

void TerrainGlWidget::mouseMoveEvent(QMouseEvent* event) {
	const QPoint delta = event->pos() - lastMousePos_;
	lastMousePos_ = event->pos();

	if (rotating_) {
		yawDeg_ -= delta.x() * 0.28f;
		pitchDeg_ += delta.y() * 0.22f;
		pitchDeg_ = std::clamp(pitchDeg_, 8.0f, 85.0f);
		update();
		event->accept();
		return;
	}

	if (panning_) {
		const float yawRad = qDegreesToRadians(yawDeg_);
		const float pitchRad = qDegreesToRadians(pitchDeg_);
		const QVector3D forward(
			std::cos(pitchRad) * std::cos(yawRad),
			std::cos(pitchRad) * std::sin(yawRad),
			std::sin(pitchRad));
		const QVector3D worldUp(0.0f, 0.0f, 1.0f);
		QVector3D right = QVector3D::crossProduct(forward, worldUp).normalized();
		QVector3D up = QVector3D::crossProduct(right, forward).normalized();
		const float panScale = distance_ * 0.0014f;
		target_ -= right * static_cast<float>(delta.x()) * panScale;
		target_ += up * static_cast<float>(delta.y()) * panScale;
		update();
		event->accept();
		return;
	}

	QOpenGLWidget::mouseMoveEvent(event);
}

void TerrainGlWidget::mouseReleaseEvent(QMouseEvent* event) {
	if (event->button() == Qt::RightButton) rotating_ = false;
	if (event->button() == Qt::RightButton) panning_ = false;
	QOpenGLWidget::mouseReleaseEvent(event);
}

void TerrainGlWidget::wheelEvent(QWheelEvent* event) {
	const QPoint numDegrees = event->angleDelta() / 8;
	if (!numDegrees.isNull()) {
		const float steps = static_cast<float>(numDegrees.y()) / 15.0f;
		const float scale = std::pow(0.88f, steps);
		distance_ *= scale;
		distance_ = std::clamp(distance_, 120.0f, 500000.0f);
		update();
	}
	event->accept();
}
