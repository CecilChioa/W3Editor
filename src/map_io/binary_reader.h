#pragma once

#include <QByteArray>
#include <QRectF>
#include <QString>

#include <cstring>

class BinaryReader {
public:
	explicit BinaryReader(const QByteArray& data) : data_(data) {}

	bool ok() const { return ok_; }
	qsizetype position() const { return offset_; }
	qsizetype remaining() const { return data_.size() - offset_; }

	bool skip(qsizetype bytes) {
		if (bytes < 0 || remaining() < bytes) {
			ok_ = false;
			return false;
		}
		offset_ += bytes;
		return true;
	}

	bool skipCString() {
		if (remaining() <= 0) {
			ok_ = false;
			return false;
		}
		while (offset_ < data_.size() && data_[offset_] != '\0') {
			++offset_;
		}
		if (offset_ >= data_.size()) {
			ok_ = false;
			return false;
		}
		++offset_;
		return true;
	}

	quint8 u8() {
		if (remaining() < 1) {
			ok_ = false;
			return 0;
		}
		return static_cast<quint8>(data_[offset_++]);
	}

	quint16 u16() {
		if (remaining() < 2) {
			ok_ = false;
			return 0;
		}
		const auto* p = bytes();
		offset_ += 2;
		return static_cast<quint16>(p[0]) |
			(static_cast<quint16>(p[1]) << 8);
	}

	quint32 u32() {
		if (remaining() < 4) {
			ok_ = false;
			return 0;
		}
		const auto* p = bytes();
		offset_ += 4;
		return static_cast<quint32>(p[0]) |
			(static_cast<quint32>(p[1]) << 8) |
			(static_cast<quint32>(p[2]) << 16) |
			(static_cast<quint32>(p[3]) << 24);
	}

	qint32 i32() {
		return static_cast<qint32>(u32());
	}

	float f32() {
		const quint32 bits = u32();
		float value = 0.0f;
		static_assert(sizeof(value) == sizeof(bits));
		memcpy(&value, &bits, sizeof(value));
		return value;
	}

	QString fourcc() {
		if (remaining() < 4) {
			ok_ = false;
			return {};
		}
		const QString value = QString::fromLatin1(data_.constData() + offset_, 4);
		offset_ += 4;
		return value;
	}

	QString cstring() {
		if (remaining() <= 0) {
			ok_ = false;
			return {};
		}
		const qsizetype start = offset_;
		while (offset_ < data_.size() && data_[offset_] != '\0') {
			++offset_;
		}
		if (offset_ >= data_.size()) {
			ok_ = false;
			return {};
		}
		const QString value = QString::fromUtf8(data_.constData() + start, offset_ - start);
		++offset_;
		return value;
	}

private:
	const unsigned char* bytes() const {
		return reinterpret_cast<const unsigned char*>(data_.constData() + offset_);
	}

	const QByteArray& data_;
	qsizetype offset_ = 0;
	bool ok_ = true;
};
