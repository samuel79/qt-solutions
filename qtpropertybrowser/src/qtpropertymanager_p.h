/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt Solutions component.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#pragma once

#include "qtpropertybrowser.h"
#include "qtpropertybrowserutils_p.h"
#include "qtpropertymanager.h"
#include <QMetaEnum>
#include <QDateTime>
#include <QLocale>
#include <QMap>
#include <QIcon>
#include <QLineEdit>
#include <QVector3D>

#if QT_VERSION >= 0x040400
QT_BEGIN_NAMESPACE
#endif

class QDate;
class QTime;
class QDateTime;
class QLocale;
class QTimer;

template <class PrivateData, class Value>
static void setSimpleMinimumData(PrivateData *data, const Value &minVal)
{
	data->minVal = minVal;
	if( data->maxVal < data->minVal )
		data->maxVal = data->minVal;

	if( data->val < data->minVal )
		data->val = data->minVal;
}

template <class PrivateData, class Value>
static void setSimpleMaximumData(PrivateData *data, const Value &maxVal)
{
	data->maxVal = maxVal;
	if( data->minVal > data->maxVal )
		data->minVal = data->maxVal;

	if( data->val > data->maxVal )
		data->val = data->maxVal;
}

template <class PrivateData, class Value>
static void setSizeMinimumData(PrivateData *data, const Value &newMinVal)
{
	data->minVal = newMinVal;
	if( data->maxVal.width() < data->minVal.width() )
		data->maxVal.setWidth(data->minVal.width());
	if( data->maxVal.height() < data->minVal.height() )
		data->maxVal.setHeight(data->minVal.height());

	if( data->val.width() < data->minVal.width() )
		data->val.setWidth(data->minVal.width());
	if( data->val.height() < data->minVal.height() )
		data->val.setHeight(data->minVal.height());
}

template <class PrivateData, class Value>
static void setSizeMaximumData(PrivateData *data, const Value &newMaxVal)
{
	data->maxVal = newMaxVal;
	if( data->minVal.width() > data->maxVal.width() )
		data->minVal.setWidth(data->maxVal.width());
	if( data->minVal.height() > data->maxVal.height() )
		data->minVal.setHeight(data->maxVal.height());

	if( data->val.width() > data->maxVal.width() )
		data->val.setWidth(data->maxVal.width());
	if( data->val.height() > data->maxVal.height() )
		data->val.setHeight(data->maxVal.height());
}

template <class SizeValue>
static SizeValue qBoundSize(const SizeValue &minVal, const SizeValue &val, const SizeValue &maxVal)
{
	SizeValue croppedVal = val;
	if( minVal.width() > val.width() )
		croppedVal.setWidth(minVal.width());
	else if( maxVal.width() < val.width() )
		croppedVal.setWidth(maxVal.width());

	if( minVal.height() > val.height() )
		croppedVal.setHeight(minVal.height());
	else if( maxVal.height() < val.height() )
		croppedVal.setHeight(maxVal.height());

	return croppedVal;
}

// Match the exact signature of qBound for VS 6.
inline QSize qBound(QSize minVal, QSize val, QSize maxVal)
{
	return qBoundSize(minVal, val, maxVal);
}

inline QSizeF qBound(QSizeF minVal, QSizeF val, QSizeF maxVal)
{
	return qBoundSize(minVal, val, maxVal);
}

namespace {

	namespace {
		template <class Value>
		void orderBorders(Value &minVal, Value &maxVal)
		{
			if( minVal > maxVal )
				qSwap(minVal, maxVal);
		}

		template <class Value>
		static void orderSizeBorders(Value &minVal, Value &maxVal)
		{
			Value fromSize = minVal;
			Value toSize = maxVal;
			if( fromSize.width() > toSize.width() ) {
				fromSize.setWidth(maxVal.width());
				toSize.setWidth(minVal.width());
			}
			if( fromSize.height() > toSize.height() ) {
				fromSize.setHeight(maxVal.height());
				toSize.setHeight(minVal.height());
			}
			minVal = fromSize;
			maxVal = toSize;
		}

		inline void orderBorders(QSize &minVal, QSize &maxVal)
		{
			orderSizeBorders(minVal, maxVal);
		}

		inline void orderBorders(QSizeF &minVal, QSizeF &maxVal)
		{
			orderSizeBorders(minVal, maxVal);
		}

	}
}
////////

template <class Value, class PrivateData>
static Value getData(const QMap<const QtProperty *, PrivateData> &propertyMap,
	Value PrivateData::*data,
	const QtProperty *property, const Value &defaultValue = Value())
{
	typedef QMap<const QtProperty *, PrivateData> PropertyToData;
	typedef typename PropertyToData::const_iterator PropertyToDataConstIterator;
	const PropertyToDataConstIterator it = propertyMap.constFind(property);
	if( it == propertyMap.constEnd() )
		return defaultValue;
	return it.value().*data;
}

template <class Value, class PrivateData>
static Value getValue(const QMap<const QtProperty *, PrivateData> &propertyMap,
	const QtProperty *property, const Value &defaultValue = Value())
{
	return getData<Value>(propertyMap, &PrivateData::val, property, defaultValue);
}

template <class Value, class PrivateData>
static Value getMinimum(const QMap<const QtProperty *, PrivateData> &propertyMap,
	const QtProperty *property, const Value &defaultValue = Value())
{
	return getData<Value>(propertyMap, &PrivateData::minVal, property, defaultValue);
}

template <class Value, class PrivateData>
static Value getMaximum(const QMap<const QtProperty *, PrivateData> &propertyMap,
	const QtProperty *property, const Value &defaultValue = Value())
{
	return getData<Value>(propertyMap, &PrivateData::maxVal, property, defaultValue);
}

template <class ValueChangeParameter, class Value, class PropertyManager>
static void setSimpleValue(QMap<const QtProperty *, Value> &propertyMap,
	PropertyManager *manager,
	void (PropertyManager::*propertyChangedSignal)(QtProperty *),
	void (PropertyManager::*valueChangedSignal)(QtProperty *, ValueChangeParameter),
	void (QtAbstractPropertyManager::*valueChangedSignal2)(QtProperty *, const QVariant&),
	QtProperty *property, const Value &val)
{
	typedef QMap<const QtProperty *, Value> PropertyToData;
	typedef typename PropertyToData::iterator PropertyToDataIterator;
	const PropertyToDataIterator it = propertyMap.find(property);
	if( it == propertyMap.end() )
		return;

	if( it.value() == val )
		return;

	it.value() = val;

	emit(manager->*propertyChangedSignal)(property);
	emit(manager->*valueChangedSignal)(property, val);
	emit(manager->*valueChangedSignal2)(property, QVariant(val));
}

template <class ValueChangeParameter, class PropertyManagerPrivate, class PropertyManager, class Value>
static void setValueInRange(PropertyManager *manager, PropertyManagerPrivate *managerPrivate,
	void (PropertyManager::*propertyChangedSignal)(QtProperty *),
	void (PropertyManager::*valueChangedSignal)(QtProperty *, ValueChangeParameter),
	void (QtAbstractPropertyManager::*valueChangedSignal2)(QtProperty *, const QVariant&),
	QtProperty *property, const Value &val,
	void (PropertyManagerPrivate::*setSubPropertyValue)(QtProperty *, ValueChangeParameter))
{
	typedef typename PropertyManagerPrivate::Data PrivateData;
	typedef QMap<const QtProperty *, PrivateData> PropertyToData;
	typedef typename PropertyToData::iterator PropertyToDataIterator;
	const PropertyToDataIterator it = managerPrivate->m_values.find(property);
	if( it == managerPrivate->m_values.end() )
		return;

	PrivateData &data = it.value();

	if( data.val == val )
		return;

	const Value oldVal = data.val;

	data.val = qBound(data.minVal, val, data.maxVal);

	if( data.val == oldVal )
		return;

	if( setSubPropertyValue )
		(managerPrivate->*setSubPropertyValue)(property, data.val);

	emit(manager->*propertyChangedSignal)(property);
	emit(manager->*valueChangedSignal)(property, data.val);
	emit(manager->*valueChangedSignal2)(property, QVariant(data.val));
}

template <class ValueChangeParameter, class PropertyManagerPrivate, class PropertyManager, class Value>
static void setBorderValues(PropertyManager *manager, PropertyManagerPrivate *managerPrivate,
	void (PropertyManager::*propertyChangedSignal)(QtProperty *),
	void (PropertyManager::*valueChangedSignal)(QtProperty *, ValueChangeParameter),
	void (PropertyManager::*rangeChangedSignal)(QtProperty *, ValueChangeParameter, ValueChangeParameter),
	QtProperty *property, const Value &minVal, const Value &maxVal,
	void (PropertyManagerPrivate::*setSubPropertyRange)(QtProperty *,
	ValueChangeParameter, ValueChangeParameter, ValueChangeParameter))
{
	typedef typename PropertyManagerPrivate::Data PrivateData;
	typedef QMap<const QtProperty *, PrivateData> PropertyToData;
	typedef typename PropertyToData::iterator PropertyToDataIterator;
	const PropertyToDataIterator it = managerPrivate->m_values.find(property);
	if( it == managerPrivate->m_values.end() )
		return;

	Value fromVal = minVal;
	Value toVal = maxVal;
	orderBorders(fromVal, toVal);

	PrivateData &data = it.value();

	if( data.minVal == fromVal && data.maxVal == toVal )
		return;

	const Value oldVal = data.val;

	data.setMinimumValue(fromVal);
	data.setMaximumValue(toVal);

	emit(manager->*rangeChangedSignal)(property, data.minVal, data.maxVal);

	if( setSubPropertyRange )
		(managerPrivate->*setSubPropertyRange)(property, data.minVal, data.maxVal, data.val);

	if( data.val == oldVal )
		return;

	emit(manager->*propertyChangedSignal)(property);
	emit(manager->*valueChangedSignal)(property, data.val);
}

template <class ValueChangeParameter, class PropertyManagerPrivate, class PropertyManager, class Value, class PrivateData>
static void setBorderValue(PropertyManager *manager, PropertyManagerPrivate *managerPrivate,
	void (PropertyManager::*propertyChangedSignal)(QtProperty *),
	void (PropertyManager::*valueChangedSignal)(QtProperty *, ValueChangeParameter),
	void (PropertyManager::*rangeChangedSignal)(QtProperty *, ValueChangeParameter, ValueChangeParameter),
	QtProperty *property,
	Value(PrivateData::*getRangeVal)() const,
	void (PrivateData::*setRangeVal)(ValueChangeParameter), const Value &borderVal,
	void (PropertyManagerPrivate::*setSubPropertyRange)(QtProperty *,
	ValueChangeParameter, ValueChangeParameter, ValueChangeParameter))
{
	typedef QMap<const QtProperty *, PrivateData> PropertyToData;
	typedef typename PropertyToData::iterator PropertyToDataIterator;
	const PropertyToDataIterator it = managerPrivate->m_values.find(property);
	if( it == managerPrivate->m_values.end() )
		return;

	PrivateData &data = it.value();

	if( (data.*getRangeVal)() == borderVal )
		return;

	const Value oldVal = data.val;

	(data.*setRangeVal)(borderVal);

	emit(manager->*rangeChangedSignal)(property, data.minVal, data.maxVal);

	if( setSubPropertyRange )
		(managerPrivate->*setSubPropertyRange)(property, data.minVal, data.maxVal, data.val);

	if( data.val == oldVal )
		return;

	emit(manager->*propertyChangedSignal)(property);
	emit(manager->*valueChangedSignal)(property, data.val);
}

template <class ValueChangeParameter, class PropertyManagerPrivate, class PropertyManager, class Value, class PrivateData>
static void setMinimumValue(PropertyManager *manager, PropertyManagerPrivate *managerPrivate,
	void (PropertyManager::*propertyChangedSignal)(QtProperty *),
	void (PropertyManager::*valueChangedSignal)(QtProperty *, ValueChangeParameter),
	void (PropertyManager::*rangeChangedSignal)(QtProperty *, ValueChangeParameter, ValueChangeParameter),
	QtProperty *property, const Value &minVal)
{
	void (PropertyManagerPrivate::*setSubPropertyRange)(QtProperty *,
		ValueChangeParameter, ValueChangeParameter, ValueChangeParameter) = 0;
	setBorderValue<ValueChangeParameter, PropertyManagerPrivate, PropertyManager, Value, PrivateData>(manager, managerPrivate,
		propertyChangedSignal, valueChangedSignal, rangeChangedSignal,
		property, &PropertyManagerPrivate::Data::minimumValue, &PropertyManagerPrivate::Data::setMinimumValue, minVal, setSubPropertyRange);
}

template <class ValueChangeParameter, class PropertyManagerPrivate, class PropertyManager, class Value, class PrivateData>
static void setMaximumValue(PropertyManager *manager, PropertyManagerPrivate *managerPrivate,
	void (PropertyManager::*propertyChangedSignal)(QtProperty *),
	void (PropertyManager::*valueChangedSignal)(QtProperty *, ValueChangeParameter),
	void (PropertyManager::*rangeChangedSignal)(QtProperty *, ValueChangeParameter, ValueChangeParameter),
	QtProperty *property, const Value &maxVal)
{
	void (PropertyManagerPrivate::*setSubPropertyRange)(QtProperty *,
		ValueChangeParameter, ValueChangeParameter, ValueChangeParameter) = 0;
	setBorderValue<ValueChangeParameter, PropertyManagerPrivate, PropertyManager, Value, PrivateData>(manager, managerPrivate,
		propertyChangedSignal, valueChangedSignal, rangeChangedSignal,
		property, &PropertyManagerPrivate::Data::maximumValue, &PropertyManagerPrivate::Data::setMaximumValue, maxVal, setSubPropertyRange);
}

class QtMetaEnumWrapper : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QSizePolicy::Policy policy READ policy)
public:
    QSizePolicy::Policy policy() const { return QSizePolicy::Ignored; }
private:
    QtMetaEnumWrapper(QObject *parent) : QObject(parent) {}
};

class QtMetaEnumProvider
{
public:
    QtMetaEnumProvider();

    QStringList policyEnumNames() const { return m_policyEnumNames; }
    QStringList languageEnumNames() const { return m_languageEnumNames; }
    QStringList countryEnumNames(QLocale::Language language) const { return m_countryEnumNames.value(language); }

    QSizePolicy::Policy indexToSizePolicy(int index) const;
    int sizePolicyToIndex(QSizePolicy::Policy policy) const;

    void indexToLocale(int languageIndex, int countryIndex, QLocale::Language *language, QLocale::Country *country) const;
    void localeToIndex(QLocale::Language language, QLocale::Country country, int *languageIndex, int *countryIndex) const;

private:
    void initLocale();

    QStringList m_policyEnumNames;
    QStringList m_languageEnumNames;
    QMap<QLocale::Language, QStringList> m_countryEnumNames;
    QMap<int, QLocale::Language> m_indexToLanguage;
    QMap<QLocale::Language, int> m_languageToIndex;
    QMap<int, QMap<int, QLocale::Country> > m_indexToCountry;
    QMap<QLocale::Language, QMap<QLocale::Country, int> > m_countryToIndex;
    QMetaEnum m_policyEnum;
};


class QtIntPropertyManager;

class QtIntPropertyManagerPrivate
{
	QtIntPropertyManager *q_ptr;
	Q_DECLARE_PUBLIC(QtIntPropertyManager)
public:

	struct Data
	{
		Data() : val(0), minVal(-INT_MAX), maxVal(INT_MAX), singleStep(1), readOnly(false) {}
		int val;
		int minVal;
		int maxVal;
		int singleStep;
		bool readOnly;
		int minimumValue() const { return minVal; }
		int maximumValue() const { return maxVal; }
		void setMinimumValue(int newMinVal) { setSimpleMinimumData(this, newMinVal); }
		void setMaximumValue(int newMaxVal) { setSimpleMaximumData(this, newMaxVal); }
	};

	typedef QMap<const QtProperty *, Data> PropertyValueMap;
	PropertyValueMap m_values;
};


class QtDoublePropertyManager;

class QtDoublePropertyManagerPrivate
{
	QtDoublePropertyManager *q_ptr;
	Q_DECLARE_PUBLIC(QtDoublePropertyManager)
public:

	struct Data
	{
		Data() : val(0), minVal(-INT_MAX), maxVal(INT_MAX), singleStep(1), decimals(2), readOnly(false) {}
		double val;
		double minVal;
		double maxVal;
		double singleStep;
		int decimals;
		bool readOnly;
		double minimumValue() const { return minVal; }
		double maximumValue() const { return maxVal; }
		void setMinimumValue(double newMinVal) { setSimpleMinimumData(this, newMinVal); }
		void setMaximumValue(double newMaxVal) { setSimpleMaximumData(this, newMaxVal); }
	};

	typedef QMap<const QtProperty *, Data> PropertyValueMap;
	PropertyValueMap m_values;
};


class QtDoubleNPropertyManager;

class QtDoubleNPropertyManagerPrivate
{
	QtDoubleNPropertyManager *q_ptr;
	Q_DECLARE_PUBLIC(QtDoubleNPropertyManager)
public:

	struct Data
	{
		QtDoubleN val;
		double minVal = -INT_MAX;
		double maxVal = INT_MAX;
		double singleStep = 1;
		int decimals = 2;
		bool readOnly = false;
		double minimumValue() const { return minVal; }
		double maximumValue() const { return maxVal; }
		void setMinimumValue(double newMinVal) {
			this->minVal = newMinVal;
			if( this->maxVal < this->minVal ) {
				this->maxVal = this->minVal;
			}
			for( auto i=0; i < this->val.n; i++ ) {
				if( this->val.val[i] < this->minVal ) {
					this->val.val[i] = this->minVal;
				}
			}
		}
		void setMaximumValue(double newMaxVal) {
			this->maxVal = newMaxVal;
			if( this->minVal > this->maxVal ) {
				this->minVal = this->maxVal;
			}
			for( auto i=0; i < this->val.n; i++ ) {
				if( this->val.val[i] > this->maxVal ) {
					this->val.val[i] = this->maxVal;
				}
			}
		}
	};

	void slotDoubleChanged(QtProperty *property, double value);
	void slotPropertyDestroyed(QtProperty *property);

	typedef QMap<const QtProperty *, Data> PropertyValueMap;
	PropertyValueMap m_values;

	QtDoublePropertyManager *m_doublePropertyManager;

	QMap<const QtProperty *, QtProperty *> m_propertyToField[QtDoubleN::MAX_SIZE];
	QMap<const QtProperty *, QtProperty *> m_fieldToProperty[QtDoubleN::MAX_SIZE];
};



class QtIntNPropertyManager;

class QtIntNPropertyManagerPrivate
{
	QtIntNPropertyManager *q_ptr;
	Q_DECLARE_PUBLIC(QtIntNPropertyManager)
public:

	struct Data
	{
		QtIntN val;
		int minVal = -INT_MAX;
		int maxVal = INT_MAX;
		int singleStep = 1;
		bool readOnly = false;
		int minimumValue() const { return minVal; }
		int maximumValue() const { return maxVal; }
		void setMinimumValue(int newMinVal) {
			this->minVal = newMinVal;
			if( this->maxVal < this->minVal ) {
				this->maxVal = this->minVal;
			}
			for( auto i=0; i < this->val.n; i++ ) {
				if( this->val.val[i] < this->minVal ) {
					this->val.val[i] = this->minVal;
				}
			}
		}
		void setMaximumValue(int newMaxVal) {
			this->maxVal = newMaxVal;
			if( this->minVal > this->maxVal ) {
				this->minVal = this->maxVal;
			}
			for( auto i=0; i < this->val.n; i++ ) {
				if( this->val.val[i] > this->maxVal ) {
					this->val.val[i] = this->maxVal;
				}
			}
		}
	};

	void slotIntChanged(QtProperty *property, int value);
	void slotPropertyDestroyed(QtProperty *property);

	typedef QMap<const QtProperty *, Data> PropertyValueMap;
	PropertyValueMap m_values;

	QtIntPropertyManager *m_intPropertyManager;

	QMap<const QtProperty *, QtProperty *> m_propertyToField[QtIntN::MAX_SIZE];
	QMap<const QtProperty *, QtProperty *> m_fieldToProperty[QtIntN::MAX_SIZE];
};


class QtColorFPropertyManager;

class QtColorFPropertyManagerPrivate
{
	QtColorFPropertyManager *q_ptr;
	Q_DECLARE_PUBLIC(QtColorFPropertyManager)
public:

	struct Data
	{
		QtColorF val;
		bool alphaHidden = false;
	};

	void slotDoubleChanged(QtProperty *property, double value);
	void slotPropertyDestroyed(QtProperty *property);

	typedef QMap<const QtProperty *, Data> PropertyValueMap;
	PropertyValueMap m_values;

	QtDoublePropertyManager *m_doublePropertyManager;

	QMap<const QtProperty *, QtProperty *> m_propertyToField[4];
	QMap<const QtProperty *, QtProperty *> m_fieldToProperty[4];
};


class QtStringPropertyManager;

class QtStringPropertyManagerPrivate
{
	QtStringPropertyManager *q_ptr;
	Q_DECLARE_PUBLIC(QtStringPropertyManager)
public:

	struct Data
	{
		Data() : regExp(QString(QLatin1Char('*')), Qt::CaseSensitive, QRegExp::Wildcard),
			echoMode(QLineEdit::Normal), readOnly(false)
		{
		}
		QString val;
		QRegExp regExp;
		int echoMode;
		bool readOnly;
	};

	typedef QMap<const QtProperty *, Data> PropertyValueMap;
	QMap<const QtProperty *, Data> m_values;
};


class QtBoolPropertyManager;

class QtBoolPropertyManagerPrivate
{
	QtBoolPropertyManager *q_ptr;
	Q_DECLARE_PUBLIC(QtBoolPropertyManager)
public:
	QtBoolPropertyManagerPrivate();

	struct Data
	{
		Data() : val(false), textVisible(true) {}
		bool val;
		bool textVisible;
	};

	typedef QMap<const QtProperty *, Data> PropertyValueMap;
	PropertyValueMap m_values;

	const QIcon m_checkedIcon;
	const QIcon m_uncheckedIcon;
};

class QtDatePropertyManager;
class QtDatePropertyManagerPrivate
{
	QtDatePropertyManager *q_ptr;
	Q_DECLARE_PUBLIC(QtDatePropertyManager)
public:

	struct Data
	{
		Data() : val(QDate::currentDate()), minVal(QDate(1752, 9, 14)),
			maxVal(QDate(7999, 12, 31)) {}
		QDate val;
		QDate minVal;
		QDate maxVal;
		QDate minimumValue() const { return minVal; }
		QDate maximumValue() const { return maxVal; }
		void setMinimumValue(const QDate &newMinVal) { setSimpleMinimumData(this, newMinVal); }
		void setMaximumValue(const QDate &newMaxVal) { setSimpleMaximumData(this, newMaxVal); }
	};

	QString m_format;

	typedef QMap<const QtProperty *, Data> PropertyValueMap;
	QMap<const QtProperty *, Data> m_values;
};

class QtTimePropertyManager;
class QtTimePropertyManagerPrivate
{
	QtTimePropertyManager *q_ptr;
	Q_DECLARE_PUBLIC(QtTimePropertyManager)
public:

	QString m_format;

	typedef QMap<const QtProperty *, QTime> PropertyValueMap;
	PropertyValueMap m_values;
};

class QtDateTimePropertyManager;

class QtDateTimePropertyManagerPrivate
{
	QtDateTimePropertyManager *q_ptr;
	Q_DECLARE_PUBLIC(QtDateTimePropertyManager)
public:

	QString m_format;

	typedef QMap<const QtProperty *, QDateTime> PropertyValueMap;
	PropertyValueMap m_values;
};


class QtKeySequencePropertyManager;
class QtKeySequencePropertyManagerPrivate
{
	QtKeySequencePropertyManager *q_ptr;
	Q_DECLARE_PUBLIC(QtKeySequencePropertyManager)
public:

	QString m_format;

	typedef QMap<const QtProperty *, QKeySequence> PropertyValueMap;
	PropertyValueMap m_values;
};


class QtCharPropertyManager;
class QtCharPropertyManagerPrivate
{
	QtCharPropertyManager *q_ptr;
	Q_DECLARE_PUBLIC(QtCharPropertyManager)
public:

	typedef QMap<const QtProperty *, QChar> PropertyValueMap;
	PropertyValueMap m_values;
};


class QtLocalePropertyManager;
class QtEnumPropertyManager;
class QtLocalePropertyManagerPrivate
{
	QtLocalePropertyManager *q_ptr;
	Q_DECLARE_PUBLIC(QtLocalePropertyManager)
public:

	QtLocalePropertyManagerPrivate();

	void slotEnumChanged(QtProperty *property, int value);
	void slotPropertyDestroyed(QtProperty *property);

	typedef QMap<const QtProperty *, QLocale> PropertyValueMap;
	PropertyValueMap m_values;

	QtEnumPropertyManager *m_enumPropertyManager;

	QMap<const QtProperty *, QtProperty *> m_propertyToLanguage;
	QMap<const QtProperty *, QtProperty *> m_propertyToCountry;

	QMap<const QtProperty *, QtProperty *> m_languageToProperty;
	QMap<const QtProperty *, QtProperty *> m_countryToProperty;
};



class QtPointPropertyManager;
class QtPointPropertyManagerPrivate
{
	QtPointPropertyManager *q_ptr;
	Q_DECLARE_PUBLIC(QtPointPropertyManager)
public:

	void slotIntChanged(QtProperty *property, int value);
	void slotPropertyDestroyed(QtProperty *property);

	typedef QMap<const QtProperty *, QPoint> PropertyValueMap;
	PropertyValueMap m_values;

	QtIntPropertyManager *m_intPropertyManager;

	QMap<const QtProperty *, QtProperty *> m_propertyToX;
	QMap<const QtProperty *, QtProperty *> m_propertyToY;

	QMap<const QtProperty *, QtProperty *> m_xToProperty;
	QMap<const QtProperty *, QtProperty *> m_yToProperty;
};


class QtPointFPropertyManager;
class QtPointFPropertyManagerPrivate
{
	QtPointFPropertyManager *q_ptr;
	Q_DECLARE_PUBLIC(QtPointFPropertyManager)
public:

	struct Data
	{
		Data() : decimals(2) {}
		QPointF val;
		int decimals;
	};

	void slotDoubleChanged(QtProperty *property, double value);
	void slotPropertyDestroyed(QtProperty *property);

	typedef QMap<const QtProperty *, Data> PropertyValueMap;
	PropertyValueMap m_values;

	QtDoublePropertyManager *m_doublePropertyManager;

	QMap<const QtProperty *, QtProperty *> m_propertyToX;
	QMap<const QtProperty *, QtProperty *> m_propertyToY;

	QMap<const QtProperty *, QtProperty *> m_xToProperty;
	QMap<const QtProperty *, QtProperty *> m_yToProperty;
};


class QtSizePropertyManager;
class QtSizePropertyManagerPrivate
{
	QtSizePropertyManager *q_ptr;
	Q_DECLARE_PUBLIC(QtSizePropertyManager)
public:

	void slotIntChanged(QtProperty *property, int value);
	void slotPropertyDestroyed(QtProperty *property);
	void setValue(QtProperty *property, const QSize &val);
	void setRange(QtProperty *property,
		const QSize &minVal, const QSize &maxVal, const QSize &val);

	struct Data
	{
		Data() : val(QSize(0, 0)), minVal(QSize(0, 0)), maxVal(QSize(INT_MAX, INT_MAX)) {}
		QSize val;
		QSize minVal;
		QSize maxVal;
		QSize minimumValue() const { return minVal; }
		QSize maximumValue() const { return maxVal; }
		void setMinimumValue(const QSize &newMinVal) { setSizeMinimumData(this, newMinVal); }
		void setMaximumValue(const QSize &newMaxVal) { setSizeMaximumData(this, newMaxVal); }
	};

	typedef QMap<const QtProperty *, Data> PropertyValueMap;
	PropertyValueMap m_values;

	QtIntPropertyManager *m_intPropertyManager;

	QMap<const QtProperty *, QtProperty *> m_propertyToW;
	QMap<const QtProperty *, QtProperty *> m_propertyToH;

	QMap<const QtProperty *, QtProperty *> m_wToProperty;
	QMap<const QtProperty *, QtProperty *> m_hToProperty;
};

class QtSizeFPropertyManager;
class QtSizeFPropertyManagerPrivate
{
	QtSizeFPropertyManager *q_ptr;
	Q_DECLARE_PUBLIC(QtSizeFPropertyManager)
public:

	void slotDoubleChanged(QtProperty *property, double value);
	void slotPropertyDestroyed(QtProperty *property);
	void setValue(QtProperty *property, const QSizeF &val);
	void setRange(QtProperty *property,
		const QSizeF &minVal, const QSizeF &maxVal, const QSizeF &val);

	struct Data
	{
		Data() : val(QSizeF(0, 0)), minVal(QSizeF(0, 0)), maxVal(QSizeF(INT_MAX, INT_MAX)), decimals(2) {}
		QSizeF val;
		QSizeF minVal;
		QSizeF maxVal;
		int decimals;
		QSizeF minimumValue() const { return minVal; }
		QSizeF maximumValue() const { return maxVal; }
		void setMinimumValue(const QSizeF &newMinVal) { setSizeMinimumData(this, newMinVal); }
		void setMaximumValue(const QSizeF &newMaxVal) { setSizeMaximumData(this, newMaxVal); }
	};

	typedef QMap<const QtProperty *, Data> PropertyValueMap;
	PropertyValueMap m_values;

	QtDoublePropertyManager *m_doublePropertyManager;

	QMap<const QtProperty *, QtProperty *> m_propertyToW;
	QMap<const QtProperty *, QtProperty *> m_propertyToH;

	QMap<const QtProperty *, QtProperty *> m_wToProperty;
	QMap<const QtProperty *, QtProperty *> m_hToProperty;
};

class QtRectPropertyManager;
class QtRectPropertyManagerPrivate
{
	QtRectPropertyManager *q_ptr;
	Q_DECLARE_PUBLIC(QtRectPropertyManager)
public:

	void slotIntChanged(QtProperty *property, int value);
	void slotPropertyDestroyed(QtProperty *property);
	void setConstraint(QtProperty *property, const QRect &constraint, const QRect &val);

	struct Data
	{
		Data() : val(0, 0, 0, 0) {}
		QRect val;
		QRect constraint;
	};

	typedef QMap<const QtProperty *, Data> PropertyValueMap;
	PropertyValueMap m_values;

	QtIntPropertyManager *m_intPropertyManager;

	QMap<const QtProperty *, QtProperty *> m_propertyToX;
	QMap<const QtProperty *, QtProperty *> m_propertyToY;
	QMap<const QtProperty *, QtProperty *> m_propertyToW;
	QMap<const QtProperty *, QtProperty *> m_propertyToH;

	QMap<const QtProperty *, QtProperty *> m_xToProperty;
	QMap<const QtProperty *, QtProperty *> m_yToProperty;
	QMap<const QtProperty *, QtProperty *> m_wToProperty;
	QMap<const QtProperty *, QtProperty *> m_hToProperty;
};

class QtRectFPropertyManager;
class QtRectFPropertyManagerPrivate
{
	QtRectFPropertyManager *q_ptr;
	Q_DECLARE_PUBLIC(QtRectFPropertyManager)
public:

	void slotDoubleChanged(QtProperty *property, double value);
	void slotPropertyDestroyed(QtProperty *property);
	void setConstraint(QtProperty *property, const QRectF &constraint, const QRectF &val);

	struct Data
	{
		Data() : val(0, 0, 0, 0), decimals(2) {}
		QRectF val;
		QRectF constraint;
		int decimals;
	};

	typedef QMap<const QtProperty *, Data> PropertyValueMap;
	PropertyValueMap m_values;

	QtDoublePropertyManager *m_doublePropertyManager;

	QMap<const QtProperty *, QtProperty *> m_propertyToX;
	QMap<const QtProperty *, QtProperty *> m_propertyToY;
	QMap<const QtProperty *, QtProperty *> m_propertyToW;
	QMap<const QtProperty *, QtProperty *> m_propertyToH;

	QMap<const QtProperty *, QtProperty *> m_xToProperty;
	QMap<const QtProperty *, QtProperty *> m_yToProperty;
	QMap<const QtProperty *, QtProperty *> m_wToProperty;
	QMap<const QtProperty *, QtProperty *> m_hToProperty;
};


class QtVector3DPropertyManager;
class QtVector3DPropertyManagerPrivate {
    QtVector3DPropertyManager *q_ptr;
    Q_DECLARE_PUBLIC(QtVector3DPropertyManager)
public:

    struct Data {
        Data() : decimals(3) {}
        QVector3D val;
        int decimals;
    };

    void slotDoubleChanged(QtProperty *property, double value);
    void slotPropertyDestroyed(QtProperty *property);

    typedef QMap<const QtProperty *, Data> PropertyValueMap;
    PropertyValueMap m_values;

    QtDoublePropertyManager *m_doublePropertyManager;

    QMap<const QtProperty *, QtProperty *> m_propertyToX;
    QMap<const QtProperty *, QtProperty *> m_propertyToY;
    QMap<const QtProperty *, QtProperty *> m_propertyToZ;

    QMap<const QtProperty *, QtProperty *> m_xToProperty;
    QMap<const QtProperty *, QtProperty *> m_yToProperty;
    QMap<const QtProperty *, QtProperty *> m_zToProperty;
};



class QtEnumPropertyManager;
class QtEnumPropertyManagerPrivate
{
	QtEnumPropertyManager *q_ptr;
	Q_DECLARE_PUBLIC(QtEnumPropertyManager)
public:

	struct Data
	{
		Data() : val(-1) {}
		int val;
		QStringList enumNames;
		QVariantList enumValues;
		QMap<int, QIcon> enumIcons;
	};

	typedef QMap<const QtProperty *, Data> PropertyValueMap;
	PropertyValueMap m_values;
};


class QtFlagPropertyManager;
class QtFlagPropertyManagerPrivate
{
	QtFlagPropertyManager *q_ptr;
	Q_DECLARE_PUBLIC(QtFlagPropertyManager)
public:

	void slotBoolChanged(QtProperty *property, bool value);
	void slotPropertyDestroyed(QtProperty *property);

	struct Data
	{
		Data() : val(-1) {}
		int val;
		QStringList flagNames;
	};

	typedef QMap<const QtProperty *, Data> PropertyValueMap;
	PropertyValueMap m_values;

	QtBoolPropertyManager *m_boolPropertyManager;

	QMap<const QtProperty *, QList<QtProperty *> > m_propertyToFlags;

	QMap<const QtProperty *, QtProperty *> m_flagToProperty;
};


class QtSizePolicyPropertyManager;
class QtSizePolicyPropertyManagerPrivate
{
	QtSizePolicyPropertyManager *q_ptr;
	Q_DECLARE_PUBLIC(QtSizePolicyPropertyManager)
public:

	QtSizePolicyPropertyManagerPrivate();

	void slotIntChanged(QtProperty *property, int value);
	void slotEnumChanged(QtProperty *property, int value);
	void slotPropertyDestroyed(QtProperty *property);

	typedef QMap<const QtProperty *, QSizePolicy> PropertyValueMap;
	PropertyValueMap m_values;

	QtIntPropertyManager *m_intPropertyManager;
	QtEnumPropertyManager *m_enumPropertyManager;

	QMap<const QtProperty *, QtProperty *> m_propertyToHPolicy;
	QMap<const QtProperty *, QtProperty *> m_propertyToVPolicy;
	QMap<const QtProperty *, QtProperty *> m_propertyToHStretch;
	QMap<const QtProperty *, QtProperty *> m_propertyToVStretch;

	QMap<const QtProperty *, QtProperty *> m_hPolicyToProperty;
	QMap<const QtProperty *, QtProperty *> m_vPolicyToProperty;
	QMap<const QtProperty *, QtProperty *> m_hStretchToProperty;
	QMap<const QtProperty *, QtProperty *> m_vStretchToProperty;
};




class QtFontPropertyManager;
class QtFontPropertyManagerPrivate
{
	QtFontPropertyManager *q_ptr;
	Q_DECLARE_PUBLIC(QtFontPropertyManager)
public:

	QtFontPropertyManagerPrivate();

	void slotIntChanged(QtProperty *property, int value);
	void slotEnumChanged(QtProperty *property, int value);
	void slotBoolChanged(QtProperty *property, bool value);
	void slotPropertyDestroyed(QtProperty *property);
	void slotFontDatabaseChanged();
	void slotFontDatabaseDelayedChange();

	QStringList m_familyNames;

	typedef QMap<const QtProperty *, QFont> PropertyValueMap;
	PropertyValueMap m_values;

	QtIntPropertyManager *m_intPropertyManager;
	QtEnumPropertyManager *m_enumPropertyManager;
	QtBoolPropertyManager *m_boolPropertyManager;

	QMap<const QtProperty *, QtProperty *> m_propertyToFamily;
	QMap<const QtProperty *, QtProperty *> m_propertyToPointSize;
	QMap<const QtProperty *, QtProperty *> m_propertyToBold;
	QMap<const QtProperty *, QtProperty *> m_propertyToItalic;
	QMap<const QtProperty *, QtProperty *> m_propertyToUnderline;
	QMap<const QtProperty *, QtProperty *> m_propertyToStrikeOut;
	QMap<const QtProperty *, QtProperty *> m_propertyToKerning;

	QMap<const QtProperty *, QtProperty *> m_familyToProperty;
	QMap<const QtProperty *, QtProperty *> m_pointSizeToProperty;
	QMap<const QtProperty *, QtProperty *> m_boldToProperty;
	QMap<const QtProperty *, QtProperty *> m_italicToProperty;
	QMap<const QtProperty *, QtProperty *> m_underlineToProperty;
	QMap<const QtProperty *, QtProperty *> m_strikeOutToProperty;
	QMap<const QtProperty *, QtProperty *> m_kerningToProperty;

	bool m_settingValue;
	QTimer *m_fontDatabaseChangeTimer;
};

class QtColorPropertyManager;
class QtColorPropertyManagerPrivate
{
	QtColorPropertyManager *q_ptr;
	Q_DECLARE_PUBLIC(QtColorPropertyManager)
public:

	void slotIntChanged(QtProperty *property, int value);
	void slotPropertyDestroyed(QtProperty *property);

	typedef QMap<const QtProperty *, QColor> PropertyValueMap;
	PropertyValueMap m_values;
	QMap<const QtProperty *, bool> m_alphaHiddenMap;
	
	QtIntPropertyManager *m_intPropertyManager;

	QMap<const QtProperty *, QtProperty *> m_propertyToR;
	QMap<const QtProperty *, QtProperty *> m_propertyToG;
	QMap<const QtProperty *, QtProperty *> m_propertyToB;
	QMap<const QtProperty *, QtProperty *> m_propertyToA;

	QMap<const QtProperty *, QtProperty *> m_rToProperty;
	QMap<const QtProperty *, QtProperty *> m_gToProperty;
	QMap<const QtProperty *, QtProperty *> m_bToProperty;
	QMap<const QtProperty *, QtProperty *> m_aToProperty;
};


class QtCursorPropertyManager;
class QtCursorPropertyManagerPrivate
{
	QtCursorPropertyManager *q_ptr;
	Q_DECLARE_PUBLIC(QtCursorPropertyManager)
public:
	typedef QMap<const QtProperty *, QCursor> PropertyValueMap;
	PropertyValueMap m_values;
};

#if QT_VERSION >= 0x040400
QT_END_NAMESPACE
#endif




