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


#include "qtpropertymanager.h"
#include "qtpropertybrowserutils_p.h"
#include <QTimer>
#include <QMetaEnum>
#include <QFontDatabase>
#include <QStyleOption>
#include <QStyle>
#include <QApplication>
#include <QPainter>
#include <QLabel>
#include <QCheckBox>
#include <QVector3D>

#include <limits.h>
#include <float.h>

#if QT_VERSION >= 0x040400
QT_BEGIN_NAMESPACE
#endif


static QList<QLocale::Country> sortCountries(const QList<QLocale::Country> &countries)
{
    QMultiMap<QString, QLocale::Country> nameToCountry;
    QListIterator<QLocale::Country> itCountry(countries);
    while (itCountry.hasNext()) {
        QLocale::Country country = itCountry.next();
        nameToCountry.insert(QLocale::countryToString(country), country);
    }
    return nameToCountry.values();
}

void QtMetaEnumProvider::initLocale()
{
    QMultiMap<QString, QLocale::Language> nameToLanguage;
    QLocale::Language language = QLocale::C;
    while (language <= QLocale::LastLanguage) {
        QLocale locale(language);
        if (locale.language() == language)
            nameToLanguage.insert(QLocale::languageToString(language), language);
        language = (QLocale::Language)((uint)language + 1); // ++language
    }

    const QLocale system = QLocale::system();
    if (!nameToLanguage.contains(QLocale::languageToString(system.language())))
        nameToLanguage.insert(QLocale::languageToString(system.language()), system.language());

    QList<QLocale::Language> languages = nameToLanguage.values();
    QListIterator<QLocale::Language> itLang(languages);
    while (itLang.hasNext()) {
        QLocale::Language language = itLang.next();
        QList<QLocale::Country> countries;
        countries = QLocale::countriesForLanguage(language);
        if (countries.isEmpty() && language == system.language())
            countries << system.country();

        if (!countries.isEmpty() && !m_languageToIndex.contains(language)) {
            countries = sortCountries(countries);
            int langIdx = m_languageEnumNames.count();
            m_indexToLanguage[langIdx] = language;
            m_languageToIndex[language] = langIdx;
            QStringList countryNames;
            QListIterator<QLocale::Country> it(countries);
            int countryIdx = 0;
            while (it.hasNext()) {
                QLocale::Country country = it.next();
                countryNames << QLocale::countryToString(country);
                m_indexToCountry[langIdx][countryIdx] = country;
                m_countryToIndex[language][country] = countryIdx;
                ++countryIdx;
            }
            m_languageEnumNames << QLocale::languageToString(language);
            m_countryEnumNames[language] = countryNames;
        }
    }
}

QtMetaEnumProvider::QtMetaEnumProvider()
{
    QMetaProperty p;

    p = QtMetaEnumWrapper::staticMetaObject.property(
                QtMetaEnumWrapper::staticMetaObject.propertyOffset() + 0);
    m_policyEnum = p.enumerator();
    const int keyCount = m_policyEnum.keyCount();
    for (int i = 0; i < keyCount; i++)
        m_policyEnumNames << QLatin1String(m_policyEnum.key(i));

    initLocale();
}

QSizePolicy::Policy QtMetaEnumProvider::indexToSizePolicy(int index) const
{
    return static_cast<QSizePolicy::Policy>(m_policyEnum.value(index));
}

int QtMetaEnumProvider::sizePolicyToIndex(QSizePolicy::Policy policy) const
{
     const int keyCount = m_policyEnum.keyCount();
    for (int i = 0; i < keyCount; i++)
        if (indexToSizePolicy(i) == policy)
            return i;
    return -1;
}

void QtMetaEnumProvider::indexToLocale(int languageIndex, int countryIndex, QLocale::Language *language, QLocale::Country *country) const
{
    QLocale::Language l = QLocale::C;
    QLocale::Country c = QLocale::AnyCountry;
    if (m_indexToLanguage.contains(languageIndex)) {
        l = m_indexToLanguage[languageIndex];
        if (m_indexToCountry.contains(languageIndex) && m_indexToCountry[languageIndex].contains(countryIndex))
            c = m_indexToCountry[languageIndex][countryIndex];
    }
    if (language)
        *language = l;
    if (country)
        *country = c;
}

void QtMetaEnumProvider::localeToIndex(QLocale::Language language, QLocale::Country country, int *languageIndex, int *countryIndex) const
{
    int l = -1;
    int c = -1;
    if (m_languageToIndex.contains(language)) {
        l = m_languageToIndex[language];
        if (m_countryToIndex.contains(language) && m_countryToIndex[language].contains(country))
            c = m_countryToIndex[language][country];
    }

    if (languageIndex)
        *languageIndex = l;
    if (countryIndex)
        *countryIndex = c;
}

Q_GLOBAL_STATIC(QtMetaEnumProvider, metaEnumProvider)

// QtGroupPropertyManager

/*!
    \class QtGroupPropertyManager

    \brief The QtGroupPropertyManager provides and manages group properties.

    This class is intended to provide a grouping element without any value.

    \sa QtAbstractPropertyManager
*/

/*!
    Creates a manager with the given \a parent.
*/
QtGroupPropertyManager::QtGroupPropertyManager(QObject *parent)
    : QtAbstractPropertyManager(parent)
{

}

/*!
    Destroys this manager, and all the properties it has created.
*/
QtGroupPropertyManager::~QtGroupPropertyManager()
{

}


QVariant QtGroupPropertyManager::attributeValue(const QtProperty *property, const QString &attribute) const
{
	Q_UNUSED(property);
	qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	return QVariant();
}


int QtGroupPropertyManager::propertyTypeId() const
{
	return qMetaTypeId<QtGroupPropertyType>();
}


int QtGroupPropertyManager::valueTypeId() const
{
	return QVariant::Invalid;
}


QVariant QtGroupPropertyManager::variantValue(const QtProperty *property) const
{
	Q_UNUSED(property);
	return QVariant();
}


void QtGroupPropertyManager::setValue(QtProperty *property, const QVariant &val)
{
	Q_UNUSED(property); Q_UNUSED(val);
}


void QtGroupPropertyManager::setAttribute(QtProperty *property, const QString &attribute, const QVariant &value)
{
	Q_UNUSED(property); Q_UNUSED(value);
	qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
}


/*!
    \reimp
*/
bool QtGroupPropertyManager::hasValue(const QtProperty *property) const
{
    Q_UNUSED(property)
    return false;
}

/*!
    \reimp
*/
void QtGroupPropertyManager::initializeProperty(QtProperty *property)
{
    Q_UNUSED(property)
}

/*!
    \reimp
*/
void QtGroupPropertyManager::uninitializeProperty(QtProperty *property)
{
    Q_UNUSED(property)
}

// QtIntPropertyManager

/*!
    \class QtIntPropertyManager

    \brief The QtIntPropertyManager provides and manages int properties.

    An int property has a current value, and a range specifying the
    valid values. The range is defined by a minimum and a maximum
    value.

    The property's value and range can be retrieved using the value(),
    minimum() and maximum() functions, and can be set using the
    setValue(), setMinimum() and setMaximum() slots. Alternatively,
    the range can be defined in one go using the setRange() slot.

    In addition, QtIntPropertyManager provides the valueChanged() signal which
    is emitted whenever a property created by this manager changes,
    and the rangeChanged() signal which is emitted whenever such a
    property changes its range of valid values.

    \sa QtAbstractPropertyManager, QtSpinBoxFactory, QtSliderFactory, QtScrollBarFactory
*/

/*!
    \fn void QtIntPropertyManager::valueChanged(QtProperty *property, int value)

    This signal is emitted whenever a property created by this manager
    changes its value, passing a pointer to the \a property and the new
    \a value as parameters.

    \sa setValue()
*/

/*!
    \fn void QtIntPropertyManager::rangeChanged(QtProperty *property, int minimum, int maximum)

    This signal is emitted whenever a property created by this manager
    changes its range of valid values, passing a pointer to the
    \a property and the new \a minimum and \a maximum values.

    \sa setRange()
*/

/*!
    \fn void QtIntPropertyManager::singleStepChanged(QtProperty *property, int step)

    This signal is emitted whenever a property created by this manager
    changes its single step property, passing a pointer to the
    \a property and the new \a step value

    \sa setSingleStep()
*/

/*!
    Creates a manager with the given \a parent.
*/
QtIntPropertyManager::QtIntPropertyManager(QObject *parent)
    : QtAbstractPropertyManager(parent)
{
    d_ptr = new QtIntPropertyManagerPrivate;
    d_ptr->q_ptr = this;
}

/*!
    Destroys this manager, and all the properties it has created.
*/
QtIntPropertyManager::~QtIntPropertyManager()
{
    clear();
    delete d_ptr;
}

/*!
    Returns the given \a property's value.

    If the given property is not managed by this manager, this
    function returns 0.

    \sa setValue()
*/
int QtIntPropertyManager::value(const QtProperty *property) const
{
    return getValue<int>(d_ptr->m_values, property, 0);
}

QVariant QtIntPropertyManager::variantValue(const QtProperty *property) const
{
	return QVariant::fromValue(value(property));
}

QVariant QtIntPropertyManager::attributeValue(const QtProperty *property, const QString &attribute) const
{
	if( attribute == "minimum" ) {
		return QVariant(minimum(property));
	} else if( attribute == "maximum" ) {
		return QVariant(maximum(property));
	} else if( attribute == "singleStep" ) {
		return QVariant(singleStep(property));
	} else if( attribute == "readOnly" ) {
		return QVariant(isReadOnly(property));
	} else {
		qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	}
	return QVariant();
}


int QtIntPropertyManager::propertyTypeId() const
{
	return QVariant::Int;
}


/*!
    Returns the given \a property's minimum value.

    \sa setMinimum(), maximum(), setRange()
*/
int QtIntPropertyManager::minimum(const QtProperty *property) const
{
    return getMinimum<int>(d_ptr->m_values, property, 0);
}

/*!
    Returns the given \a property's maximum value.

    \sa setMaximum(), minimum(), setRange()
*/
int QtIntPropertyManager::maximum(const QtProperty *property) const
{
    return getMaximum<int>(d_ptr->m_values, property, 0);
}

/*!
    Returns the given \a property's step value.

    The step is typically used to increment or decrement a property value while pressing an arrow key.

    \sa setSingleStep()
*/
int QtIntPropertyManager::singleStep(const QtProperty *property) const
{
    return getData<int>(d_ptr->m_values, &QtIntPropertyManagerPrivate::Data::singleStep, property, 0);
}

/*!
    Returns read-only status of the property.

    When property is read-only it's value can be selected and copied from editor but not modified.

    \sa QtIntPropertyManager::setReadOnly
*/
bool QtIntPropertyManager::isReadOnly(const QtProperty *property) const
{
    return getData<bool>(d_ptr->m_values, &QtIntPropertyManagerPrivate::Data::readOnly, property, false);
}




/*!
    \reimp
*/
QString QtIntPropertyManager::valueText(const QtProperty *property) const
{
    const QtIntPropertyManagerPrivate::PropertyValueMap::const_iterator it = d_ptr->m_values.constFind(property);
    if (it == d_ptr->m_values.constEnd())
        return QString();
    return QString::number(it.value().val);
}

/*!
    \fn void QtIntPropertyManager::setValue(QtProperty *property, int value)

    Sets the value of the given \a property to \a value.

    If the specified \a value is not valid according to the given \a
    property's range, the \a value is adjusted to the nearest valid
    value within the range.

    \sa value(), setRange(), valueChanged()
*/
void QtIntPropertyManager::setValue(QtProperty *property, int val)
{
    void (QtIntPropertyManagerPrivate::*setSubPropertyValue)(QtProperty *, int) = 0;
    setValueInRange<int, QtIntPropertyManagerPrivate, QtIntPropertyManager, int>(this, d_ptr,
                &QtIntPropertyManager::propertyChanged,
                &QtIntPropertyManager::valueChanged,
                &QtAbstractPropertyManager::valueChanged,
                property, val, setSubPropertyValue);
}


void QtIntPropertyManager::setValue(QtProperty *property, const QVariant &val)
{
	Q_ASSERT(val.canConvert<int>());
	auto s_val = qvariant_cast<int>(val);
	setValue(property, s_val);
}


void QtIntPropertyManager::setAttribute(QtProperty *property, const QString &attribute, const QVariant &value)
{
	if( attribute == "minimum" ) {
		setMinimum(property, qvariant_cast<int>(value));
	} else if( attribute == "maximum" ) {
		setMaximum(property, qvariant_cast<int>(value));
	} else if( attribute == "singleStep" ) {
		setSingleStep(property, qvariant_cast<int>(value));
	} else if( attribute == "readOnly" ) {
		setReadOnly(property, qvariant_cast<bool>(value));
	} else {
		qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	}
}


/*!
    Sets the minimum value for the given \a property to \a minVal.

    When setting the minimum value, the maximum and current values are
    adjusted if necessary (ensuring that the range remains valid and
    that the current value is within the range).

    \sa minimum(), setRange(), rangeChanged()
*/
void QtIntPropertyManager::setMinimum(QtProperty *property, int minVal)
{
	auto it = d_ptr->m_values.find(property);
	if( it == d_ptr->m_values.end() ) {
		return;
	}

	auto& data = it.value();

	if( data.minimumValue() == minVal ) {
		return;
	}

	const auto oldMaxVal = data.maxVal;
	const auto oldVal = data.val;

	data.setMinimumValue(minVal);

	emit rangeChanged(property, data.minVal, data.maxVal);
	emit attributeChanged(property, "minimum", data.minVal);

	if( oldMaxVal != data.maxVal ) {
		emit attributeChanged(property, "maximum", data.maxVal);
	}

	if( data.val != oldVal ) {
		emit propertyChanged(property);
		emit valueChanged(property, data.val);
		emit QtAbstractPropertyManager::valueChanged(property, data.val);
	}
}

/*!
    Sets the maximum value for the given \a property to \a maxVal.

    When setting maximum value, the minimum and current values are
    adjusted if necessary (ensuring that the range remains valid and
    that the current value is within the range).

    \sa maximum(), setRange(), rangeChanged()
*/
void QtIntPropertyManager::setMaximum(QtProperty *property, int maxVal)
{
	auto it = d_ptr->m_values.find(property);
	if( it == d_ptr->m_values.end() ) {
		return;
	}

	auto& data = it.value();

	if( data.maximumValue() == maxVal ) {
		return;
	}

	const auto oldMinVal = data.minVal;
	const auto oldVal = data.val;

	data.setMaximumValue(maxVal);

	emit rangeChanged(property, data.minVal, data.maxVal);
	emit attributeChanged(property, "maximum", data.maxVal);

	if( oldMinVal != data.minVal ) {
		emit attributeChanged(property, "minimum", data.minVal);
	}

	if( data.val != oldVal ) {
		emit propertyChanged(property);
		emit valueChanged(property, data.val);
		emit QtAbstractPropertyManager::valueChanged(property, data.val);
	}
}

/*!
    \fn void QtIntPropertyManager::setRange(QtProperty *property, int minimum, int maximum)

    Sets the range of valid values.

    This is a convenience function defining the range of valid values
    in one go; setting the \a minimum and \a maximum values for the
    given \a property with a single function call.

    When setting a new range, the current value is adjusted if
    necessary (ensuring that the value remains within range).

    \sa setMinimum(), setMaximum(), rangeChanged()
*/
void QtIntPropertyManager::setRange(QtProperty *property, int minVal, int maxVal)
{
	auto it = d_ptr->m_values.find(property);
	if( it == d_ptr->m_values.end() ) {
		return;
	}

	orderBorders(minVal, maxVal);

	auto& data = it.value();

	if( data.minimumValue() == minVal && data.maximumValue() == maxVal ) {
		return;
	}

	const auto oldMinVal = data.minVal;
	const auto oldMaxVal = data.maxVal;
	const auto oldVal = data.val;

	data.setMinimumValue(minVal);
	data.setMaximumValue(maxVal);

	emit rangeChanged(property, data.minVal, data.maxVal);

	if( oldMinVal != data.minVal ) {
		emit attributeChanged(property, "minimum", data.minVal);
	}
	if( oldMaxVal != data.maxVal ) {
		emit attributeChanged(property, "maximum", data.maxVal);
	}
	if( data.val != oldVal ) {
		emit propertyChanged(property);
		emit valueChanged(property, data.val);
		emit QtAbstractPropertyManager::valueChanged(property, data.val);
	}
}

/*!
    Sets the step value for the given \a property to \a step.

    The step is typically used to increment or decrement a property value while pressing an arrow key.

    \sa singleStep()
*/
void QtIntPropertyManager::setSingleStep(QtProperty *property, int step)
{
    const auto it = d_ptr->m_values.find(property);
	if( it == d_ptr->m_values.end() ) {
		return;
	}

    auto data = it.value();

	if( step < 0 ) {
		step = 0;
	}

	if( data.singleStep == step ) {
		return;
	}

    data.singleStep = step;

	it.value() = data;

    emit singleStepChanged(property, data.singleStep);
	emit attributeChanged(property, "singleStep", data.singleStep);
}

/*!
    Sets read-only status of the property.

    \sa QtIntPropertyManager::setReadOnly
*/
void QtIntPropertyManager::setReadOnly(QtProperty *property, bool readOnly)
{
    const QtIntPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    QtIntPropertyManagerPrivate::Data data = it.value();

    if (data.readOnly == readOnly)
        return;

    data.readOnly = readOnly;
    it.value() = data;

    emit propertyChanged(property);
    emit readOnlyChanged(property, data.readOnly);
	emit attributeChanged(property, "readOnly", data.singleStep);
}

/*!
    \reimp
*/
void QtIntPropertyManager::initializeProperty(QtProperty *property)
{
    d_ptr->m_values[property] = QtIntPropertyManagerPrivate::Data();
}

/*!
    \reimp
*/
void QtIntPropertyManager::uninitializeProperty(QtProperty *property)
{
    d_ptr->m_values.remove(property);
}

// QtDoublePropertyManager

/*!
    \class QtDoublePropertyManager

    \brief The QtDoublePropertyManager provides and manages double properties.

    A double property has a current value, and a range specifying the
    valid values. The range is defined by a minimum and a maximum
    value.

    The property's value and range can be retrieved using the value(),
    minimum() and maximum() functions, and can be set using the
    setValue(), setMinimum() and setMaximum() slots.
    Alternatively, the range can be defined in one go using the
    setRange() slot.

    In addition, QtDoublePropertyManager provides the valueChanged() signal
    which is emitted whenever a property created by this manager
    changes, and the rangeChanged() signal which is emitted whenever
    such a property changes its range of valid values.

    \sa QtAbstractPropertyManager, QtDoubleSpinBoxFactory
*/

/*!
    \fn void QtDoublePropertyManager::valueChanged(QtProperty *property, double value)

    This signal is emitted whenever a property created by this manager
    changes its value, passing a pointer to the \a property and the new
    \a value as parameters.

    \sa setValue()
*/

/*!
    \fn void QtDoublePropertyManager::rangeChanged(QtProperty *property, double minimum, double maximum)

    This signal is emitted whenever a property created by this manager
    changes its range of valid values, passing a pointer to the
    \a property and the new \a minimum and \a maximum values

    \sa setRange()
*/

/*!
    \fn void QtDoublePropertyManager::decimalsChanged(QtProperty *property, int prec)

    This signal is emitted whenever a property created by this manager
    changes its precision of value, passing a pointer to the
    \a property and the new \a prec value

    \sa setDecimals()
*/

/*!
    \fn void QtDoublePropertyManager::singleStepChanged(QtProperty *property, double step)

    This signal is emitted whenever a property created by this manager
    changes its single step property, passing a pointer to the
    \a property and the new \a step value

    \sa setSingleStep()
*/

/*!
    Creates a manager with the given \a parent.
*/
QtDoublePropertyManager::QtDoublePropertyManager(QObject *parent)
    : QtAbstractPropertyManager(parent)
{
    d_ptr = new QtDoublePropertyManagerPrivate;
    d_ptr->q_ptr = this;
}

/*!
    Destroys  this manager, and all the properties it has created.
*/
QtDoublePropertyManager::~QtDoublePropertyManager()
{
    clear();
    delete d_ptr;
}

/*!
    Returns the given \a property's value.

    If the given property is not managed by this manager, this
    function returns 0.

    \sa setValue()
*/
double QtDoublePropertyManager::value(const QtProperty *property) const
{
    return getValue<double>(d_ptr->m_values, property, 0.0);
}


QVariant QtDoublePropertyManager::variantValue(const QtProperty *property) const
{
	return QVariant::fromValue(value(property));
}


QVariant QtDoublePropertyManager::attributeValue(const QtProperty *property, const QString &attribute) const
{
	if( attribute == "minimum" ) {
		return QVariant(minimum(property));
	} else if( attribute == "maximum" ) {
		return QVariant(maximum(property));
	} else if( attribute == "singleStep" ) {
		return QVariant(singleStep(property));
	} else if( attribute == "decimals" ) {
		return QVariant(decimals(property));
	} else if( attribute == "readOnly" ) {
		return QVariant(isReadOnly(property));
	} else {
		qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	}
	return QVariant();
}


int QtDoublePropertyManager::propertyTypeId() const
{
	return QVariant::Double;
}


/*!
    Returns the given \a property's minimum value.

    \sa maximum(), setRange()
*/
double QtDoublePropertyManager::minimum(const QtProperty *property) const
{
    return getMinimum<double>(d_ptr->m_values, property, 0.0);
}

/*!
    Returns the given \a property's maximum value.

    \sa minimum(), setRange()
*/
double QtDoublePropertyManager::maximum(const QtProperty *property) const
{
    return getMaximum<double>(d_ptr->m_values, property, 0.0);
}

/*!
    Returns the given \a property's step value.

    The step is typically used to increment or decrement a property value while pressing an arrow key.

    \sa setSingleStep()
*/
double QtDoublePropertyManager::singleStep(const QtProperty *property) const
{
    return getData<double>(d_ptr->m_values, &QtDoublePropertyManagerPrivate::Data::singleStep, property, 0);
}

/*!
    Returns the given \a property's precision, in decimals.

    \sa setDecimals()
*/
int QtDoublePropertyManager::decimals(const QtProperty *property) const
{
    return getData<int>(d_ptr->m_values, &QtDoublePropertyManagerPrivate::Data::decimals, property, 0);
}

/*!
    Returns read-only status of the property.

    When property is read-only it's value can be selected and copied from editor but not modified.

    \sa QtDoublePropertyManager::setReadOnly
*/
bool QtDoublePropertyManager::isReadOnly(const QtProperty *property) const
{
    return getData<bool>(d_ptr->m_values, &QtDoublePropertyManagerPrivate::Data::readOnly, property, false);
}

/*!
    \reimp
*/
QString QtDoublePropertyManager::valueText(const QtProperty *property) const
{
    const QtDoublePropertyManagerPrivate::PropertyValueMap::const_iterator it = d_ptr->m_values.constFind(property);
    if (it == d_ptr->m_values.constEnd())
        return QString();
    return QLocale::system().toString(it.value().val, 'f', it.value().decimals);
}

/*!
    \fn void QtDoublePropertyManager::setValue(QtProperty *property, double value)

    Sets the value of the given \a property to \a value.

    If the specified \a value is not valid according to the given
    \a property's range, the \a value is adjusted to the nearest valid value
    within the range.

    \sa value(), setRange(), valueChanged()
*/
void QtDoublePropertyManager::setValue(QtProperty *property, double val)
{
    void (QtDoublePropertyManagerPrivate::*setSubPropertyValue)(QtProperty *, double) = 0;
    setValueInRange<double, QtDoublePropertyManagerPrivate, QtDoublePropertyManager, double>(this, d_ptr,
                &QtDoublePropertyManager::propertyChanged,
                &QtDoublePropertyManager::valueChanged,
                &QtAbstractPropertyManager::valueChanged,
                property, val, setSubPropertyValue);
}


void QtDoublePropertyManager::setAttribute(QtProperty *property, const QString &attribute, const QVariant &value)
{
	if( attribute == "minimum" ) {
		setMinimum(property, qvariant_cast<double>(value));
	} else if( attribute == "maximum" ) {
		setMaximum(property, qvariant_cast<double>(value));
	} else if( attribute == "singleStep" ) {
		setSingleStep(property, qvariant_cast<double>(value));
	} else if( attribute == "decimals" ) {
		setDecimals(property, qvariant_cast<int>(value));
	} else if( attribute == "readOnly" ) {
		setReadOnly(property, qvariant_cast<bool>(value));
	} else {
		qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	}
}


void QtDoublePropertyManager::setValue(QtProperty *property, const QVariant &val)
{
	Q_ASSERT(val.canConvert<double>());
	auto s_val = qvariant_cast<double>(val);
	setValue(property, s_val);
}


/*!
    Sets the step value for the given \a property to \a step.

    The step is typically used to increment or decrement a property value while pressing an arrow key.

    \sa singleStep()
*/
void QtDoublePropertyManager::setSingleStep(QtProperty *property, double step)
{
    const QtDoublePropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    QtDoublePropertyManagerPrivate::Data data = it.value();

    if (step < 0)
        step = 0;

    if (data.singleStep == step)
        return;

    data.singleStep = step;

    it.value() = data;

    emit singleStepChanged(property, data.singleStep);
	emit attributeChanged(property, "singleStep", data.singleStep);
}

/*!
    Sets read-only status of the property.

    \sa QtDoublePropertyManager::setReadOnly
*/
void QtDoublePropertyManager::setReadOnly(QtProperty *property, bool readOnly)
{
    const QtDoublePropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    QtDoublePropertyManagerPrivate::Data data = it.value();

    if (data.readOnly == readOnly)
        return;

    data.readOnly = readOnly;
    it.value() = data;

    emit propertyChanged(property);
    emit readOnlyChanged(property, data.readOnly);
	emit attributeChanged(property, "readOnly", data.readOnly);
}

/*!
    \fn void QtDoublePropertyManager::setDecimals(QtProperty *property, int prec)

    Sets the precision of the given \a property to \a prec.

    The valid decimal range is 0-13. The default is 2.

    \sa decimals()
*/
void QtDoublePropertyManager::setDecimals(QtProperty *property, int prec)
{
    const QtDoublePropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    QtDoublePropertyManagerPrivate::Data data = it.value();

    if (prec > 13)
        prec = 13;
    else if (prec < 0)
        prec = 0;

    if (data.decimals == prec)
        return;

    data.decimals = prec;

    it.value() = data;

    emit decimalsChanged(property, data.decimals);
	emit attributeChanged(property, "decimals", data.decimals);
}

/*!
    Sets the minimum value for the given \a property to \a minVal.

    When setting the minimum value, the maximum and current values are
    adjusted if necessary (ensuring that the range remains valid and
    that the current value is within in the range).

    \sa minimum(), setRange(), rangeChanged()
*/
void QtDoublePropertyManager::setMinimum(QtProperty *property, double minVal)
{
	auto it = d_ptr->m_values.find(property);
	if( it == d_ptr->m_values.end() ) {
		return;
	}

	auto& data = it.value();

	if( data.minimumValue() == minVal ) {
		return;
	}

	const auto oldMaxVal = data.maxVal;
	const auto oldVal = data.val;

	data.setMinimumValue(minVal);

	emit rangeChanged(property, data.minVal, data.maxVal);
	emit attributeChanged(property, "minimum", data.minVal);

	if( oldMaxVal != data.maxVal ) {
		emit attributeChanged(property, "maximum", data.maxVal);
	}

	if( data.val != oldVal ) {
		emit propertyChanged(property);
		emit valueChanged(property, data.val);
		emit QtAbstractPropertyManager::valueChanged(property, data.val);
	}
}

/*!
    Sets the maximum value for the given \a property to \a maxVal.

    When setting the maximum value, the minimum and current values are
    adjusted if necessary (ensuring that the range remains valid and
    that the current value is within in the range).

    \sa maximum(), setRange(), rangeChanged()
*/
void QtDoublePropertyManager::setMaximum(QtProperty *property, double maxVal)
{
	auto it = d_ptr->m_values.find(property);
	if( it == d_ptr->m_values.end() ) {
		return;
	}

	auto& data = it.value();

	if( data.maximumValue() == maxVal ) {
		return;
	}

	const auto oldMinVal = data.minVal;
	const auto oldVal = data.val;

	data.setMaximumValue(maxVal);

	emit rangeChanged(property, data.minVal, data.maxVal);
	emit attributeChanged(property, "maximum", data.maxVal);

	if( oldMinVal != data.minVal ) {
		emit attributeChanged(property, "minimum", data.minVal);
	}

	if( data.val != oldVal ) {
		emit propertyChanged(property);
		emit valueChanged(property, data.val);
		emit QtAbstractPropertyManager::valueChanged(property, data.val);
	}
}

/*!
    \fn void QtDoublePropertyManager::setRange(QtProperty *property, double minimum, double maximum)

    Sets the range of valid values.

    This is a convenience function defining the range of valid values
    in one go; setting the \a minimum and \a maximum values for the
    given \a property with a single function call.

    When setting a new range, the current value is adjusted if
    necessary (ensuring that the value remains within range).

    \sa setMinimum(), setMaximum(), rangeChanged()
*/
void QtDoublePropertyManager::setRange(QtProperty *property, double minVal, double maxVal)
{
	auto it = d_ptr->m_values.find(property);
	if( it == d_ptr->m_values.end() ) {
		return;
	}

	orderBorders(minVal, maxVal);

	auto& data = it.value();

	if( data.minimumValue() == minVal && data.maximumValue() == maxVal ) {
		return;
	}

	const auto oldMinVal = data.minVal;
	const auto oldMaxVal = data.maxVal;
	const auto oldVal = data.val;

	data.setMinimumValue(minVal);
	data.setMaximumValue(maxVal);

	emit rangeChanged(property, data.minVal, data.maxVal);

	if( oldMinVal != data.minVal ) {
		emit attributeChanged(property, "minimum", data.minVal);
	}
	if( oldMaxVal != data.maxVal ) {
		emit attributeChanged(property, "maximum", data.maxVal);
	}
	if( data.val != oldVal ) {
		emit propertyChanged(property);
		emit valueChanged(property, data.val);
		emit QtAbstractPropertyManager::valueChanged(property, data.val);
	}
}

/*!
    \reimp
*/
void QtDoublePropertyManager::initializeProperty(QtProperty *property)
{
    d_ptr->m_values[property] = QtDoublePropertyManagerPrivate::Data();
}

/*!
    \reimp
*/
void QtDoublePropertyManager::uninitializeProperty(QtProperty *property)
{
    d_ptr->m_values.remove(property);
}



// QtDoubleNPropertyManager

void QtDoubleNPropertyManagerPrivate::slotDoubleChanged(QtProperty *property, double value)
{
	for( auto i=0; i<QtDoubleN::MAX_SIZE; i++ ) {
		if( QtProperty *prop = m_fieldToProperty[i].value(property, 0) ) {
			auto p = m_values[prop].val;
			p.val[i] = value;
			q_ptr->setValue(prop, p);
			break;
		}
    } 
}

void QtDoubleNPropertyManagerPrivate::slotPropertyDestroyed(QtProperty *property)
{
	for( auto i=0; i<QtDoubleN::MAX_SIZE; i++ ) {
		if( QtProperty *subprop  = m_fieldToProperty[i].value(property, 0) ) {
			m_propertyToField[i][subprop] = 0;
			m_fieldToProperty[i].remove(property);
			break;
		}
    } 
}


/*!
\class QtDouble2PropertyManager

\brief The QtDouble2PropertyManager provides and manages double properties.

A double property has a current value, and a range specifying the
valid values. The range is defined by a minimum and a maximum
value.

The property's value and range can be retrieved using the value(),
minimum() and maximum() functions, and can be set using the
setValue(), setMinimum() and setMaximum() slots.
Alternatively, the range can be defined in one go using the
setRange() slot.

In addition, QtDouble2PropertyManager provides the valueChanged() signal
which is emitted whenever a property created by this manager
changes, and the rangeChanged() signal which is emitted whenever
such a property changes its range of valid values.

\sa QtAbstractPropertyManager, QtDouble2SpinBoxFactory
*/

/*!
\fn void QtDouble2PropertyManager::valueChanged(QtProperty *property, double value)

This signal is emitted whenever a property created by this manager
changes its value, passing a pointer to the \a property and the new
\a value as parameters.

\sa setValue()
*/

/*!
\fn void QtDouble2PropertyManager::rangeChanged(QtProperty *property, double minimum, double maximum)

This signal is emitted whenever a property created by this manager
changes its range of valid values, passing a pointer to the
\a property and the new \a minimum and \a maximum values

\sa setRange()
*/

/*!
\fn void QtDouble2PropertyManager::decimalsChanged(QtProperty *property, int prec)

This signal is emitted whenever a property created by this manager
changes its precision of value, passing a pointer to the
\a property and the new \a prec value

\sa setDecimals()
*/

/*!
\fn void QtDouble2PropertyManager::singleStepChanged(QtProperty *property, double step)

This signal is emitted whenever a property created by this manager
changes its single step property, passing a pointer to the
\a property and the new \a step value

\sa setSingleStep()
*/

/*!
Creates a manager with the given \a parent.
*/
QtDoubleNPropertyManager::QtDoubleNPropertyManager(QObject *parent)
	: QtAbstractPropertyManager(parent)
{
	d_ptr = new QtDoubleNPropertyManagerPrivate;
	d_ptr->q_ptr = this;

    d_ptr->m_doublePropertyManager = new QtDoublePropertyManager(this);
    connect(d_ptr->m_doublePropertyManager, SIGNAL(valueChanged(QtProperty *, double)),
                this, SLOT(slotDoubleChanged(QtProperty *, double)));
    connect(d_ptr->m_doublePropertyManager, SIGNAL(propertyDestroyed(QtProperty *)),
                this, SLOT(slotPropertyDestroyed(QtProperty *)));
}

/*!
Destroys  this manager, and all the properties it has created.
*/
QtDoubleNPropertyManager::~QtDoubleNPropertyManager()
{
	clear();
	delete d_ptr;
}

/*!
    Returns the manager that creates the nested \e x and \e y
    subproperties.

    In order to provide editing widgets for the subproperties in a
    property browser widget, this manager must be associated with an
    editor factory.

    \sa QtAbstractPropertyBrowser::setFactoryForManager()
*/
QtDoublePropertyManager *QtDoubleNPropertyManager::subDoublePropertyManager() const
{
    return d_ptr->m_doublePropertyManager;
}

/*!
Returns the given \a property's value.

If the given property is not managed by this manager, this
function returns 0.

\sa setValue()
*/
QtDoubleN QtDoubleNPropertyManager::value(const QtProperty *property) const
{
	return getValue<QtDoubleN>(d_ptr->m_values, property);
}


QVariant QtDoubleNPropertyManager::variantValue(const QtProperty *property) const
{
	return QVariant::fromValue(value(property));
}


QVariant QtDoubleNPropertyManager::attributeValue(const QtProperty *property, const QString &attribute) const
{
	if( attribute == "minimum" ) {
		return QVariant(minimum(property));
	} else if( attribute == "maximum" ) {
		return QVariant(maximum(property));
	} else if( attribute == "singleStep" ) {
		return QVariant(singleStep(property));
	} else if( attribute == "decimals" ) {
		return QVariant(decimals(property));
	} else if( attribute == "readOnly" ) {
		return QVariant(isReadOnly(property));
	} else {
		qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	}
	return QVariant();
}


int QtDoubleNPropertyManager::propertyTypeId() const
{
	return qtDoubleNTypeId();
}

QList<QtAbstractPropertyManager*> QtDoubleNPropertyManager::subPropertyManagers() const
{
	return{ subDoublePropertyManager() };
}

/*!
Returns the given \a property's minimum value.

\sa maximum(), setRange()
*/
double QtDoubleNPropertyManager::minimum(const QtProperty *property) const
{
	return getMinimum<double>(d_ptr->m_values, property, 0.0);
}

/*!
Returns the given \a property's maximum value.

\sa minimum(), setRange()
*/
double QtDoubleNPropertyManager::maximum(const QtProperty *property) const
{
	return getMaximum<double>(d_ptr->m_values, property, std::numeric_limits<double>::max());
}

/*!
Returns the given \a property's step value.

The step is typically used to increment or decrement a property value while pressing an arrow key.

\sa setSingleStep()
*/
double QtDoubleNPropertyManager::singleStep(const QtProperty *property) const
{
	return getData<double>(d_ptr->m_values, &QtDoubleNPropertyManagerPrivate::Data::singleStep, property, 0);
}

/*!
Returns the given \a property's precision, in decimals.

\sa setDecimals()
*/
int QtDoubleNPropertyManager::decimals(const QtProperty *property) const
{
	return getData<int>(d_ptr->m_values, &QtDoubleNPropertyManagerPrivate::Data::decimals, property, 0);
}

/*!
Returns read-only status of the property.

When property is read-only it's value can be selected and copied from editor but not modified.

\sa QtDouble2PropertyManager::setReadOnly
*/
bool QtDoubleNPropertyManager::isReadOnly(const QtProperty *property) const
{
	return getData<bool>(d_ptr->m_values, &QtDoubleNPropertyManagerPrivate::Data::readOnly, property, false);
}

/*!
\reimp
*/
QString QtDoubleNPropertyManager::valueText(const QtProperty *property) const
{
	auto it = d_ptr->m_values.constFind(property);
	if( it == d_ptr->m_values.constEnd() )
		return QString();

	const QtDoubleN v = it.value().val;
    const int       dec =  it.value().decimals;
	int             n = v.n;

	if( n <= 0 ) {
		return QString();
	}

	QString         text = "(";
	int             i;
	for( i=0; i < n - 1; i++ ) {
		text += QString::number(v.val[i], 'f', dec) + ", ";
	}
	if( i < n ) {
		text += QString::number(v.val[i], 'f', dec) + ")";
	}

	return text;
}

/*!
\fn void QtDouble2PropertyManager::setValue(QtProperty *property, double value)

Sets the value of the given \a property to \a value.

If the specified \a value is not valid according to the given
\a property's range, the \a value is adjusted to the nearest valid value
within the range.

\sa value(), setRange(), valueChanged()
*/
void QtDoubleNPropertyManager::setValue(QtProperty *property, const QtDoubleN& val)
{
    auto it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

	auto old_val = it.value().val;

	if( old_val == val ) {
		return;
	}

	if( old_val.n != val.n ) {
		for( auto i=0; i < old_val.n; i++ ) {
			property->removeSubProperty(d_ptr->m_propertyToField[i][property]);
		}
		for( auto i=0; i < val.n; i++ ) {
			property->addSubProperty(d_ptr->m_propertyToField[i][property]);
		}
	}

	auto new_val = val;
	for( auto i=0; i<val.n; i++ ) {
		new_val.val[i] = qBound(it.value().minVal, val.val[i], it.value().maxVal);
	}

    it.value().val = new_val;

	for( auto i=0; i<new_val.n; i++ ) {
		d_ptr->m_doublePropertyManager->setValue(d_ptr->m_propertyToField[i][property], new_val.val[i]);
	}

    emit propertyChanged(property);
    emit valueChanged(property, new_val);
	emit QtAbstractPropertyManager::valueChanged(property, QVariant::fromValue(new_val));
}

void QtDoubleNPropertyManager::setAttribute(QtProperty *property, const QString &attribute, const QVariant &value)
{
	if( attribute == "minimum" ) {
		setMinimum(property, qvariant_cast<double>(value));
	} else if( attribute == "maximum" ) {
		setMaximum(property, qvariant_cast<double>(value));
	} else if( attribute == "singleStep" ) {
		setSingleStep(property, qvariant_cast<double>(value));
	} else if( attribute == "decimals" ) {
		setDecimals(property, qvariant_cast<int>(value));
	} else if( attribute == "readOnly" ) {
		setReadOnly(property, qvariant_cast<bool>(value));
	} else {
		qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	}
}


void QtDoubleNPropertyManager::setValue(QtProperty *property, const QVariant &val)
{
	Q_ASSERT(val.canConvert<QtDoubleN>());
	auto s_val = qvariant_cast<QtDoubleN>(val);
	setValue(property, s_val);
}


/*!
Sets the step value for the given \a property to \a step.

The step is typically used to increment or decrement a property value while pressing an arrow key.

\sa singleStep()
*/
void QtDoubleNPropertyManager::setSingleStep(QtProperty *property, double step)
{
	auto it = d_ptr->m_values.find(property);
	if( it == d_ptr->m_values.end() )
		return;

	QtDoubleNPropertyManagerPrivate::Data data = it.value();

	if( step < 0 )
		step = 0;

	if( data.singleStep == step )
		return;

	data.singleStep = step;

	it.value() = data;

	for( auto i=0; i<QtDoubleN::MAX_SIZE; i++ ) {
		d_ptr->m_doublePropertyManager->setSingleStep(d_ptr->m_propertyToField[i][property], data.singleStep);
	}

	emit singleStepChanged(property, data.singleStep);
	emit attributeChanged(property, "singleStep", data.singleStep);
}

/*!
Sets read-only status of the property.

\sa QtDouble2PropertyManager::setReadOnly
*/
void QtDoubleNPropertyManager::setReadOnly(QtProperty *property, bool readOnly)
{
	auto it = d_ptr->m_values.find(property);
	if( it == d_ptr->m_values.end() )
		return;

	QtDoubleNPropertyManagerPrivate::Data data = it.value();

	if( data.readOnly == readOnly )
		return;

	data.readOnly = readOnly;
	it.value() = data;

	for( auto i=0; i<QtDoubleN::MAX_SIZE; i++ ) {
		d_ptr->m_doublePropertyManager->setReadOnly(d_ptr->m_propertyToField[i][property], data.readOnly);
	}

	emit propertyChanged(property);
	emit readOnlyChanged(property, data.readOnly);
	emit attributeChanged(property, "readOnly", data.readOnly);
}

/*!
\fn void QtDouble2PropertyManager::setDecimals(QtProperty *property, int prec)

Sets the precision of the given \a property to \a prec.

The valid decimal range is 0-13. The default is 2.

\sa decimals()
*/
void QtDoubleNPropertyManager::setDecimals(QtProperty *property, int prec)
{
	const QtDoubleNPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
	if( it == d_ptr->m_values.end() )
		return;

	QtDoubleNPropertyManagerPrivate::Data data = it.value();

	if( prec > 13 )
		prec = 13;
	else if( prec < 0 )
		prec = 0;

	if( data.decimals == prec )
		return;

	data.decimals = prec;

	it.value() = data;

	for( auto i=0; i<QtDoubleN::MAX_SIZE; i++ ) {
		d_ptr->m_doublePropertyManager->setDecimals(d_ptr->m_propertyToField[i][property], data.decimals);
	}

	emit propertyChanged(property);
	emit decimalsChanged(property, data.decimals);
	emit attributeChanged(property, "decimals", data.decimals);
}

/*!
Sets the minimum value for the given \a property to \a minVal.

When setting the minimum value, the maximum and current values are
adjusted if necessary (ensuring that the range remains valid and
that the current value is within in the range).

\sa minimum(), setRange(), rangeChanged()
*/
void QtDoubleNPropertyManager::setMinimum(QtProperty *property, double minVal)
{
	auto it = d_ptr->m_values.find(property);
	if( it == d_ptr->m_values.end() ) {
		return;
	}

	auto& data = it.value();

	if( data.minimumValue() == minVal ) {
		return;
	}

	const auto oldMaxVal = data.maxVal;
	const auto oldVal = data.val;

	data.setMinimumValue(minVal);

	emit rangeChanged(property, data.minVal, data.maxVal);
	emit attributeChanged(property, "minimum", data.minVal);

	if( oldMaxVal != data.maxVal ) {
		emit attributeChanged(property, "maximum", data.maxVal);
	}

	for( auto i=0; i<QtDoubleN::MAX_SIZE; i++ ) {
		d_ptr->m_doublePropertyManager->setMinimum(d_ptr->m_propertyToField[i][property], minVal);
	}

	if( data.val != oldVal ) {
		emit propertyChanged(property);
		emit valueChanged(property, data.val);
		emit QtAbstractPropertyManager::valueChanged(property, QVariant::fromValue(data.val));
	}
}

/*!
Sets the maximum value for the given \a property to \a maxVal.

When setting the maximum value, the minimum and current values are
adjusted if necessary (ensuring that the range remains valid and
that the current value is within in the range).

\sa maximum(), setRange(), rangeChanged()
*/
void QtDoubleNPropertyManager::setMaximum(QtProperty *property, double maxVal)
{
	auto it = d_ptr->m_values.find(property);
	if( it == d_ptr->m_values.end() ) {
		return;
	}

	auto& data = it.value();

	if( data.maximumValue() == maxVal ) {
		return;
	}

	const auto oldMinVal = data.minVal;
	const auto oldVal = data.val;

	data.setMaximumValue(maxVal);

	emit rangeChanged(property, data.minVal, data.maxVal);
	emit attributeChanged(property, "maximum", data.maxVal);

	if( oldMinVal != data.minVal ) {
		emit attributeChanged(property, "minimum", data.minVal);
	}

	for( auto i=0; i<QtDoubleN::MAX_SIZE; i++ ) {
		d_ptr->m_doublePropertyManager->setMaximum(d_ptr->m_propertyToField[i][property], maxVal);
	}

	if( data.val != oldVal ) {
		emit propertyChanged(property);
		emit valueChanged(property, data.val);
		emit QtAbstractPropertyManager::valueChanged(property, QVariant::fromValue(data.val));
	}
}

/*!
\fn void QtDouble2PropertyManager::setRange(QtProperty *property, double minimum, double maximum)

Sets the range of valid values.

This is a convenience function defining the range of valid values
in one go; setting the \a minimum and \a maximum values for the
given \a property with a single function call.

When setting a new range, the current value is adjusted if
necessary (ensuring that the value remains within range).

\sa setMinimum(), setMaximum(), rangeChanged()
*/
void QtDoubleNPropertyManager::setRange(QtProperty *property, double minVal, double maxVal)
{
	auto it = d_ptr->m_values.find(property);
	if( it == d_ptr->m_values.end() ) {
		return;
	}

	orderBorders(minVal, maxVal);

	auto& data = it.value();

	if( data.minimumValue() == minVal && data.maximumValue() == maxVal ) {
		return;
	}

	const auto oldMinVal = data.minVal;
	const auto oldMaxVal = data.maxVal;
	const auto oldVal = data.val;

	data.setMinimumValue(minVal);
	data.setMaximumValue(maxVal);

	emit rangeChanged(property, data.minVal, data.maxVal);

	if( oldMinVal != data.minVal ) {
		emit attributeChanged(property, "minimum", data.minVal);
	}
	if( oldMaxVal != data.maxVal ) {
		emit attributeChanged(property, "maximum", data.maxVal);
	}

	for( auto i=0; i<QtDoubleN::MAX_SIZE; i++ ) {
		d_ptr->m_doublePropertyManager->setMinimum(d_ptr->m_propertyToField[i][property], minVal);
		d_ptr->m_doublePropertyManager->setMaximum(d_ptr->m_propertyToField[i][property], maxVal);
	}

	if( data.val != oldVal ) {
		emit propertyChanged(property);
		emit valueChanged(property, data.val);
		emit QtAbstractPropertyManager::valueChanged(property, QVariant::fromValue(data.val));
	}
}

/*!
\reimp
*/
void QtDoubleNPropertyManager::initializeProperty(QtProperty *property)
{
	d_ptr->m_values[property] = QtDoubleNPropertyManagerPrivate::Data();

	for( auto i=0; i < QtDoubleN::MAX_SIZE; i++ ) {
		QtProperty *subprop = d_ptr->m_doublePropertyManager->addProperty();
		subprop->setPropertyName(tr("[%1]").arg(i));
		d_ptr->m_doublePropertyManager->setDecimals(subprop, decimals(property));
		d_ptr->m_doublePropertyManager->setMinimum(subprop, minimum(property));
		d_ptr->m_doublePropertyManager->setMaximum(subprop, maximum(property));
		d_ptr->m_doublePropertyManager->setSingleStep(subprop, singleStep(property));
		d_ptr->m_doublePropertyManager->setValue(subprop, 0);
		d_ptr->m_propertyToField[i][property] = subprop;
		d_ptr->m_fieldToProperty[i][subprop] = property;
	}
}

/*!
\reimp
*/
void QtDoubleNPropertyManager::uninitializeProperty(QtProperty *property)
{
	for( auto i=0; i < QtDoubleN::MAX_SIZE; i++ ) {
		QtProperty *subprop = d_ptr->m_propertyToField[i][property];
		if( subprop ) {
			d_ptr->m_fieldToProperty[i].remove(subprop);
			delete subprop;
		}
		d_ptr->m_propertyToField[i].remove(property);
	}

	d_ptr->m_values.remove(property);
}


// QtIntNPropertyManager

void QtIntNPropertyManagerPrivate::slotIntChanged(QtProperty *property, int value)
{
	for( auto i=0; i<QtIntN::MAX_SIZE; i++ ) {
		if( QtProperty *prop = m_fieldToProperty[i].value(property, 0) ) {
			auto p = m_values[prop].val;
			p.val[i] = value;
			q_ptr->setValue(prop, p);
			break;
		}
    } 
}

void QtIntNPropertyManagerPrivate::slotPropertyDestroyed(QtProperty *property)
{
	for( auto i=0; i<QtIntN::MAX_SIZE; i++ ) {
		if( QtProperty *subprop  = m_fieldToProperty[i].value(property, 0) ) {
			m_propertyToField[i][subprop] = 0;
			m_fieldToProperty[i].remove(property);
			break;
		}
    } 
}


/*!
\class QtDouble2PropertyManager

\brief The QtDouble2PropertyManager provides and manages double properties.

A double property has a current value, and a range specifying the
valid values. The range is defined by a minimum and a maximum
value.

The property's value and range can be retrieved using the value(),
minimum() and maximum() functions, and can be set using the
setValue(), setMinimum() and setMaximum() slots.
Alternatively, the range can be defined in one go using the
setRange() slot.

In addition, QtDouble2PropertyManager provides the valueChanged() signal
which is emitted whenever a property created by this manager
changes, and the rangeChanged() signal which is emitted whenever
such a property changes its range of valid values.

\sa QtAbstractPropertyManager, QtDouble2SpinBoxFactory
*/

/*!
\fn void QtDouble2PropertyManager::valueChanged(QtProperty *property, double value)

This signal is emitted whenever a property created by this manager
changes its value, passing a pointer to the \a property and the new
\a value as parameters.

\sa setValue()
*/

/*!
\fn void QtDouble2PropertyManager::rangeChanged(QtProperty *property, double minimum, double maximum)

This signal is emitted whenever a property created by this manager
changes its range of valid values, passing a pointer to the
\a property and the new \a minimum and \a maximum values

\sa setRange()
*/

/*!
\fn void QtDouble2PropertyManager::decimalsChanged(QtProperty *property, int prec)

This signal is emitted whenever a property created by this manager
changes its precision of value, passing a pointer to the
\a property and the new \a prec value

\sa setDecimals()
*/

/*!
\fn void QtDouble2PropertyManager::singleStepChanged(QtProperty *property, double step)

This signal is emitted whenever a property created by this manager
changes its single step property, passing a pointer to the
\a property and the new \a step value

\sa setSingleStep()
*/

/*!
Creates a manager with the given \a parent.
*/
QtIntNPropertyManager::QtIntNPropertyManager(QObject *parent)
	: QtAbstractPropertyManager(parent)
{
	d_ptr = new QtIntNPropertyManagerPrivate;
	d_ptr->q_ptr = this;

    d_ptr->m_intPropertyManager = new QtIntPropertyManager(this);
    connect(d_ptr->m_intPropertyManager, SIGNAL(valueChanged(QtProperty *, int)),
                this, SLOT(slotIntChanged(QtProperty *, int)));
    connect(d_ptr->m_intPropertyManager, SIGNAL(propertyDestroyed(QtProperty *)),
                this, SLOT(slotPropertyDestroyed(QtProperty *)));
}

/*!
Destroys  this manager, and all the properties it has created.
*/
QtIntNPropertyManager::~QtIntNPropertyManager()
{
	clear();
	delete d_ptr;
}

/*!
    Returns the manager that creates the nested \e x and \e y
    subproperties.

    In order to provide editing widgets for the subproperties in a
    property browser widget, this manager must be associated with an
    editor factory.

    \sa QtAbstractPropertyBrowser::setFactoryForManager()
*/
QtIntPropertyManager *QtIntNPropertyManager::subIntPropertyManager() const
{
    return d_ptr->m_intPropertyManager;
}

/*!
Returns the given \a property's value.

If the given property is not managed by this manager, this
function returns 0.

\sa setValue()
*/
QtIntN QtIntNPropertyManager::value(const QtProperty *property) const
{
	return getValue<QtIntN>(d_ptr->m_values, property);
}


QVariant QtIntNPropertyManager::variantValue(const QtProperty *property) const
{
	return QVariant::fromValue(value(property));
}


QVariant QtIntNPropertyManager::attributeValue(const QtProperty *property, const QString &attribute) const
{
	if( attribute == "minimum" ) {
		return QVariant(minimum(property));
	} else if( attribute == "maximum" ) {
		return QVariant(maximum(property));
	} else if( attribute == "singleStep" ) {
		return QVariant(singleStep(property));
	} else if( attribute == "readOnly" ) {
		return QVariant(isReadOnly(property));
	} else {
		qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	}
	return QVariant();
}


int QtIntNPropertyManager::propertyTypeId() const
{
	return qtIntNTypeId();
}

QList<QtAbstractPropertyManager*> QtIntNPropertyManager::subPropertyManagers() const
{
	return{ subIntPropertyManager() };
}

/*!
Returns the given \a property's minimum value.

\sa maximum(), setRange()
*/
int QtIntNPropertyManager::minimum(const QtProperty *property) const
{
	return getMinimum<int>(d_ptr->m_values, property, 0);
}

/*!
Returns the given \a property's maximum value.

\sa minimum(), setRange()
*/
int QtIntNPropertyManager::maximum(const QtProperty *property) const
{
	return getMaximum<int>(d_ptr->m_values, property, std::numeric_limits<int>::max());
}

/*!
Returns the given \a property's step value.

The step is typically used to increment or decrement a property value while pressing an arrow key.

\sa setSingleStep()
*/
int QtIntNPropertyManager::singleStep(const QtProperty *property) const
{
	return getData<int>(d_ptr->m_values, &QtIntNPropertyManagerPrivate::Data::singleStep, property, 1);
}

/*!
Returns read-only status of the property.

When property is read-only it's value can be selected and copied from editor but not modified.

\sa QtDouble2PropertyManager::setReadOnly
*/
bool QtIntNPropertyManager::isReadOnly(const QtProperty *property) const
{
	return getData<bool>(d_ptr->m_values, &QtIntNPropertyManagerPrivate::Data::readOnly, property, false);
}

/*!
\reimp
*/
QString QtIntNPropertyManager::valueText(const QtProperty *property) const
{
	auto it = d_ptr->m_values.constFind(property);
	if( it == d_ptr->m_values.constEnd() )
		return QString();

	const QtIntN v = it.value().val;
	int          n = v.n;

	if( n <= 0 ) {
		return QString();
	}

	QString         text = "(";
	int             i;
	for( i=0; i < n - 1; i++ ) {
		text += QString::number(v.val[i]) + ", ";
	}
	if( i < n ) {
		text += QString::number(v.val[i]) + ")";
	}

	return text;
}

/*!
\fn void QtDouble2PropertyManager::setValue(QtProperty *property, double value)

Sets the value of the given \a property to \a value.

If the specified \a value is not valid according to the given
\a property's range, the \a value is adjusted to the nearest valid value
within the range.

\sa value(), setRange(), valueChanged()
*/
void QtIntNPropertyManager::setValue(QtProperty *property, const QtIntN& val)
{
    auto it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

	auto old_val = it.value().val;

	if( old_val == val ) {
		return;
	}

	if( old_val.n != val.n ) {
		for( auto i=0; i < old_val.n; i++ ) {
			property->removeSubProperty(d_ptr->m_propertyToField[i][property]);
		}
		for( auto i=0; i < val.n; i++ ) {
			property->addSubProperty(d_ptr->m_propertyToField[i][property]);
		}
	}

	auto new_val = val;
	for( auto i=0; i<val.n; i++ ) {
		new_val.val[i] = qBound(it.value().minVal, val.val[i], it.value().maxVal);
	}

    it.value().val = new_val;

	for( auto i=0; i<new_val.n; i++ ) {
		d_ptr->m_intPropertyManager->setValue(d_ptr->m_propertyToField[i][property], new_val.val[i]);
	}

    emit propertyChanged(property);
    emit valueChanged(property, new_val);
	emit QtAbstractPropertyManager::valueChanged(property, QVariant::fromValue(new_val));
}

void QtIntNPropertyManager::setAttribute(QtProperty *property, const QString &attribute, const QVariant &value)
{
	if( attribute == "minimum" ) {
		setMinimum(property, qvariant_cast<double>(value));
	} else if( attribute == "maximum" ) {
		setMaximum(property, qvariant_cast<double>(value));
	} else if( attribute == "singleStep" ) {
		setSingleStep(property, qvariant_cast<double>(value));
	} else if( attribute == "readOnly" ) {
		setReadOnly(property, qvariant_cast<bool>(value));
	} else {
		qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	}
}


void QtIntNPropertyManager::setValue(QtProperty *property, const QVariant &val)
{
	Q_ASSERT(val.canConvert<QtIntN>());
	auto s_val = qvariant_cast<QtIntN>(val);
	setValue(property, s_val);
}


/*!
Sets the step value for the given \a property to \a step.

The step is typically used to increment or decrement a property value while pressing an arrow key.

\sa singleStep()
*/
void QtIntNPropertyManager::setSingleStep(QtProperty *property, int step)
{
	auto it = d_ptr->m_values.find(property);
	if( it == d_ptr->m_values.end() )
		return;

	QtIntNPropertyManagerPrivate::Data data = it.value();

	if( step < 0 )
		step = 0;

	if( data.singleStep == step )
		return;

	data.singleStep = step;

	it.value() = data;

	for( auto i=0; i<QtIntN::MAX_SIZE; i++ ) {
		d_ptr->m_intPropertyManager->setSingleStep(d_ptr->m_propertyToField[i][property], data.singleStep);
	}

	emit singleStepChanged(property, data.singleStep);
	emit attributeChanged(property, "singleStep", data.singleStep);
}

/*!
Sets read-only status of the property.

\sa QtDouble2PropertyManager::setReadOnly
*/
void QtIntNPropertyManager::setReadOnly(QtProperty *property, bool readOnly)
{
	auto it = d_ptr->m_values.find(property);
	if( it == d_ptr->m_values.end() )
		return;

	QtIntNPropertyManagerPrivate::Data data = it.value();

	if( data.readOnly == readOnly )
		return;

	data.readOnly = readOnly;
	it.value() = data;

	for( auto i=0; i<QtIntN::MAX_SIZE; i++ ) {
		d_ptr->m_intPropertyManager->setReadOnly(d_ptr->m_propertyToField[i][property], data.readOnly);
	}

	emit propertyChanged(property);
	emit readOnlyChanged(property, data.readOnly);
	emit attributeChanged(property, "readOnly", data.readOnly);
}

/*!
Sets the minimum value for the given \a property to \a minVal.

When setting the minimum value, the maximum and current values are
adjusted if necessary (ensuring that the range remains valid and
that the current value is within in the range).

\sa minimum(), setRange(), rangeChanged()
*/
void QtIntNPropertyManager::setMinimum(QtProperty *property, int minVal)
{
	auto it = d_ptr->m_values.find(property);
	if( it == d_ptr->m_values.end() ) {
		return;
	}

	auto& data = it.value();

	if( data.minimumValue() == minVal ) {
		return;
	}

	const auto oldMaxVal = data.maxVal;
	const auto oldVal = data.val;

	data.setMinimumValue(minVal);

	emit rangeChanged(property, data.minVal, data.maxVal);
	emit attributeChanged(property, "minimum", data.minVal);

	if( oldMaxVal != data.maxVal ) {
		emit attributeChanged(property, "maximum", data.maxVal);
	}

	for( auto i=0; i<QtIntN::MAX_SIZE; i++ ) {
		d_ptr->m_intPropertyManager->setMinimum(d_ptr->m_propertyToField[i][property], minVal);
	}

	if( data.val != oldVal ) {
		emit propertyChanged(property);
		emit valueChanged(property, data.val);
		emit QtAbstractPropertyManager::valueChanged(property, QVariant::fromValue(data.val));
	}
}

/*!
Sets the maximum value for the given \a property to \a maxVal.

When setting the maximum value, the minimum and current values are
adjusted if necessary (ensuring that the range remains valid and
that the current value is within in the range).

\sa maximum(), setRange(), rangeChanged()
*/
void QtIntNPropertyManager::setMaximum(QtProperty *property, int maxVal)
{
	auto it = d_ptr->m_values.find(property);
	if( it == d_ptr->m_values.end() ) {
		return;
	}

	auto& data = it.value();

	if( data.maximumValue() == maxVal ) {
		return;
	}

	const auto oldMinVal = data.minVal;
	const auto oldVal = data.val;

	data.setMaximumValue(maxVal);

	emit rangeChanged(property, data.minVal, data.maxVal);
	emit attributeChanged(property, "maximum", data.maxVal);

	if( oldMinVal != data.minVal ) {
		emit attributeChanged(property, "minimum", data.minVal);
	}

	for( auto i=0; i<QtIntN::MAX_SIZE; i++ ) {
		d_ptr->m_intPropertyManager->setMaximum(d_ptr->m_propertyToField[i][property], maxVal);
	}

	if( data.val != oldVal ) {
		emit propertyChanged(property);
		emit valueChanged(property, data.val);
		emit QtAbstractPropertyManager::valueChanged(property, QVariant::fromValue(data.val));
	}
}

/*!
\fn void QtDouble2PropertyManager::setRange(QtProperty *property, double minimum, double maximum)

Sets the range of valid values.

This is a convenience function defining the range of valid values
in one go; setting the \a minimum and \a maximum values for the
given \a property with a single function call.

When setting a new range, the current value is adjusted if
necessary (ensuring that the value remains within range).

\sa setMinimum(), setMaximum(), rangeChanged()
*/
void QtIntNPropertyManager::setRange(QtProperty *property, int minVal, int maxVal)
{
	auto it = d_ptr->m_values.find(property);
	if( it == d_ptr->m_values.end() ) {
		return;
	}

	orderBorders(minVal, maxVal);

	auto& data = it.value();

	if( data.minimumValue() == minVal && data.maximumValue() == maxVal ) {
		return;
	}

	const auto oldMinVal = data.minVal;
	const auto oldMaxVal = data.maxVal;
	const auto oldVal = data.val;

	data.setMinimumValue(minVal);
	data.setMaximumValue(maxVal);

	emit rangeChanged(property, data.minVal, data.maxVal);

	if( oldMinVal != data.minVal ) {
		emit attributeChanged(property, "minimum", data.minVal);
	}
	if( oldMaxVal != data.maxVal ) {
		emit attributeChanged(property, "maximum", data.maxVal);
	}

	for( auto i=0; i<QtIntN::MAX_SIZE; i++ ) {
		d_ptr->m_intPropertyManager->setMinimum(d_ptr->m_propertyToField[i][property], minVal);
		d_ptr->m_intPropertyManager->setMaximum(d_ptr->m_propertyToField[i][property], maxVal);
	}

	if( data.val != oldVal ) {
		emit propertyChanged(property);
		emit valueChanged(property, data.val);
		emit QtAbstractPropertyManager::valueChanged(property, QVariant::fromValue(data.val));
	}
}

/*!
\reimp
*/
void QtIntNPropertyManager::initializeProperty(QtProperty *property)
{
	d_ptr->m_values[property] = QtIntNPropertyManagerPrivate::Data();

	for( auto i=0; i < QtIntN::MAX_SIZE; i++ ) {
		QtProperty *subprop = d_ptr->m_intPropertyManager->addProperty();
		subprop->setPropertyName(tr("[%1]").arg(i));
		d_ptr->m_intPropertyManager->setMinimum(subprop, minimum(property));
		d_ptr->m_intPropertyManager->setMaximum(subprop, maximum(property));
		d_ptr->m_intPropertyManager->setSingleStep(subprop, singleStep(property));
		d_ptr->m_intPropertyManager->setValue(subprop, 0);
		d_ptr->m_propertyToField[i][property] = subprop;
		d_ptr->m_fieldToProperty[i][subprop] = property;
	}
}

/*!
\reimp
*/
void QtIntNPropertyManager::uninitializeProperty(QtProperty *property)
{
	for( auto i=0; i < QtIntN::MAX_SIZE; i++ ) {
		QtProperty *subprop = d_ptr->m_propertyToField[i][property];
		if( subprop ) {
			d_ptr->m_fieldToProperty[i].remove(subprop);
			delete subprop;
		}
		d_ptr->m_propertyToField[i].remove(property);
	}

	d_ptr->m_values.remove(property);
}


// QtColorFPropertyManager

void QtColorFPropertyManagerPrivate::slotDoubleChanged(QtProperty *property, double value)
{
	for( auto i=0; i<q_ptr->SIZE; i++ ) {
		if( QtProperty *prop = m_fieldToProperty[i].value(property, 0) ) {
			auto p = m_values[prop].val;
			p.val[i] = value;
			q_ptr->setValue(prop, p);
			break;
		}
    } 
}

void QtColorFPropertyManagerPrivate::slotPropertyDestroyed(QtProperty *property)
{
	for( auto i=0; i<q_ptr->SIZE; i++ ) {
		if( QtProperty *pointProp  = m_fieldToProperty[i].value(property, 0) ) {
			m_propertyToField[i][pointProp] = 0;
			m_fieldToProperty[i].remove(property);
			break;
		}
    } 
}


/*!
\class QtColorFPropertyManager

\brief The QtColorFPropertyManager provides and manages double properties.

A double property has a current value, and a range specifying the
valid values. The range is defined by a minimum and a maximum
value.

The property's value and range can be retrieved using the value(),
minimum() and maximum() functions, and can be set using the
setValue(), setMinimum() and setMaximum() slots.
Alternatively, the range can be defined in one go using the
setRange() slot.

In addition, QtColorFPropertyManager provides the valueChanged() signal
which is emitted whenever a property created by this manager
changes, and the rangeChanged() signal which is emitted whenever
such a property changes its range of valid values.

\sa QtAbstractPropertyManager, QtColorFSpinBoxFactory
*/

/*!
\fn void QtColorFPropertyManager::valueChanged(QtProperty *property, double value)

This signal is emitted whenever a property created by this manager
changes its value, passing a pointer to the \a property and the new
\a value as parameters.

\sa setValue()
*/

/*!
\fn void QtColorFPropertyManager::rangeChanged(QtProperty *property, double minimum, double maximum)

This signal is emitted whenever a property created by this manager
changes its range of valid values, passing a pointer to the
\a property and the new \a minimum and \a maximum values

\sa setRange()
*/

/*!
\fn void QtColorFPropertyManager::decimalsChanged(QtProperty *property, int prec)

This signal is emitted whenever a property created by this manager
changes its precision of value, passing a pointer to the
\a property and the new \a prec value

\sa setDecimals()
*/

/*!
\fn void QtColorFPropertyManager::singleStepChanged(QtProperty *property, double step)

This signal is emitted whenever a property created by this manager
changes its single step property, passing a pointer to the
\a property and the new \a step value

\sa setSingleStep()
*/

/*!
Creates a manager with the given \a parent.
*/
QtColorFPropertyManager::QtColorFPropertyManager(QObject *parent)
	: QtAbstractPropertyManager(parent)
{
	d_ptr = new QtColorFPropertyManagerPrivate;
	d_ptr->q_ptr = this;

    d_ptr->m_doublePropertyManager = new QtDoublePropertyManager(this);
    connect(d_ptr->m_doublePropertyManager, SIGNAL(valueChanged(QtProperty *, double)),
                this, SLOT(slotDoubleChanged(QtProperty *, double)));
    connect(d_ptr->m_doublePropertyManager, SIGNAL(propertyDestroyed(QtProperty *)),
                this, SLOT(slotPropertyDestroyed(QtProperty *)));
}

/*!
Destroys  this manager, and all the properties it has created.
*/
QtColorFPropertyManager::~QtColorFPropertyManager()
{
	clear();
	delete d_ptr;
}

/*!
    Returns the manager that creates the nested \e x and \e y
    subproperties.

    In order to provide editing widgets for the subproperties in a
    property browser widget, this manager must be associated with an
    editor factory.

    \sa QtAbstractPropertyBrowser::setFactoryForManager()
*/
QtDoublePropertyManager *QtColorFPropertyManager::subDoublePropertyManager() const
{
    return d_ptr->m_doublePropertyManager;
}

/*!
Returns the given \a property's value.

If the given property is not managed by this manager, this
function returns 0.

\sa setValue()
*/
QtColorF QtColorFPropertyManager::value(const QtProperty *property) const
{
	return getValue<QtColorF>(d_ptr->m_values, property);
}


QVariant QtColorFPropertyManager::variantValue(const QtProperty *property) const
{
	return QVariant::fromValue(value(property));
}


QVariant QtColorFPropertyManager::attributeValue(const QtProperty *property, const QString &attribute) const
{
	if( attribute == "alphaHidden" ) {
		return QVariant(alphaHidden(property));
	} else {
		qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	}
	return QVariant();
}


int QtColorFPropertyManager::propertyTypeId() const
{
	return qtColorFTypeId();
}

QList<QtAbstractPropertyManager*> QtColorFPropertyManager::subPropertyManagers() const
{
	return{ subDoublePropertyManager() };
}


/*!
Returns read-only status of the property.

When property is read-only it's value can be selected and copied from editor but not modified.

\sa QtColorFPropertyManager::setReadOnly
*/
bool QtColorFPropertyManager::alphaHidden(const QtProperty *property) const
{
	return getData<bool>(d_ptr->m_values, &QtColorFPropertyManagerPrivate::Data::alphaHidden, property, false);
}


/*!
\reimp
*/
QString QtColorFPropertyManager::valueText(const QtProperty *property) const
{
	auto it = d_ptr->m_values.constFind(property);
	if( it == d_ptr->m_values.constEnd() )
		return QString();
	const QtColorF v = it.value().val;
	bool  alphaHidden = it.value().alphaHidden;

	if( alphaHidden ) {
		return QString(tr("[%1, %2, %3]")
			.arg(QString::number(v.val[0], 'f', 2))
			.arg(QString::number(v.val[1], 'f', 2))
			.arg(QString::number(v.val[2], 'f', 2)));
	} else {
		return QString(tr("[%1, %2, %3] (%4)")
			.arg(QString::number(v.val[0], 'f', 2))
			.arg(QString::number(v.val[1], 'f', 2))
			.arg(QString::number(v.val[2], 'f', 2))
			.arg(QString::number(v.val[3], 'f', 2)));
	}
}


QIcon QtColorFPropertyManager::valueIcon(const QtProperty *property) const
{
    auto it = d_ptr->m_values.constFind(property);
    if (it == d_ptr->m_values.constEnd())
        return QIcon();
	const auto& cf = it.value().val;
	QColor      color(static_cast<int>(cf.val[0] * 255),
		static_cast<int>(cf.val[1] * 255),
		static_cast<int>(cf.val[1] * 255));
	return QtPropertyBrowserUtils::brushValueIcon(QBrush(color));
}


/*!
\fn void QtColorFPropertyManager::setValue(QtProperty *property, double value)

Sets the value of the given \a property to \a value.

If the specified \a value is not valid according to the given
\a property's range, the \a value is adjusted to the nearest valid value
within the range.

\sa value(), setRange(), valueChanged()
*/
void QtColorFPropertyManager::setValue(QtProperty *property, const QtColorF& val)
{
    auto it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

	auto old_val = it.value().val;

	if( old_val.val[0] == val.val[0] && old_val.val[1] == val.val[1] &&
		old_val.val[2] == val.val[2] && old_val.val[3] == val.val[3] ) {
		return;
	}

    it.value().val = val;

	for( auto i=0; i<SIZE; i++ ) {
		d_ptr->m_doublePropertyManager->setValue(d_ptr->m_propertyToField[i][property], val.val[i]);
	}

    emit propertyChanged(property);
	emit valueChanged(property, it.value().val);
	emit QtAbstractPropertyManager::valueChanged(property, QVariant::fromValue(it.value().val));
}

void QtColorFPropertyManager::setAttribute(QtProperty *property, const QString &attribute, const QVariant &value)
{
	if( attribute == "alphaHidden" ) {
		setAlphaHidden(property, qvariant_cast<bool>(value));
	} else {
		qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	}
}


void QtColorFPropertyManager::setValue(QtProperty *property, const QVariant &val)
{
	Q_ASSERT(val.canConvert<QtColorF>());
	auto s_val = qvariant_cast<QtColorF>(val);
	setValue(property, s_val);
}


/*!
Sets read-only status of the property.

\sa QtColorFPropertyManager::setReadOnly
*/
void QtColorFPropertyManager::setAlphaHidden(QtProperty *property, bool alphaHidden)
{
	auto it = d_ptr->m_values.find(property);
	if( it == d_ptr->m_values.end() )
		return;

	QtColorFPropertyManagerPrivate::Data data = it.value();

	if( data.alphaHidden == alphaHidden ) {
		return;
	}

	data.alphaHidden = alphaHidden;
	it.value() = data;

	if( alphaHidden ) {
		property->removeSubProperty(d_ptr->m_propertyToField[SIZE-1][property]);
	} else {
		property->addSubProperty(d_ptr->m_propertyToField[SIZE-1][property]);
	}

	emit alphaHiddenChanged(property, alphaHidden);
	emit attributeChanged(property, "alphaHidden", alphaHidden);
	emit propertyChanged(property);
}

/*!
\reimp
*/
void QtColorFPropertyManager::initializeProperty(QtProperty *property)
{
	d_ptr->m_values[property] = QtColorFPropertyManagerPrivate::Data();

	QStringList labels ={ tr("Red"), tr("Green"), tr("Blue"), tr("Alpha") };
	QList<double> values ={ 0, 0, 0, 1 };

	for( auto i=0; i < SIZE; i++ ) {
		QtProperty *subprop = d_ptr->m_doublePropertyManager->addProperty();
		subprop->setPropertyName(labels[i]);
		d_ptr->m_doublePropertyManager->setDecimals(subprop, 3);
		d_ptr->m_doublePropertyManager->setMinimum(subprop, 0.0);
		d_ptr->m_doublePropertyManager->setMaximum(subprop, 1.0);
		d_ptr->m_doublePropertyManager->setSingleStep(subprop, 0.05);
		d_ptr->m_doublePropertyManager->setValue(subprop, values[i]);
		d_ptr->m_propertyToField[i][property] = subprop;
		d_ptr->m_fieldToProperty[i][subprop] = property;
		property->addSubProperty(subprop);
	}
}

/*!
\reimp
*/
void QtColorFPropertyManager::uninitializeProperty(QtProperty *property)
{
	for( auto i=0; i<SIZE; i++ ) {
		QtProperty *subprop = d_ptr->m_propertyToField[i][property];
		if( subprop ) {
			d_ptr->m_fieldToProperty[i].remove(subprop);
			delete subprop;
		}
		d_ptr->m_propertyToField[i].remove(property);
	}

	d_ptr->m_values.remove(property);
}



// QtStringPropertyManager

/*!
    \class QtStringPropertyManager

    \brief The QtStringPropertyManager provides and manages QString properties.

    A string property's value can be retrieved using the value()
    function, and set using the setValue() slot.

    The current value can be checked against a regular expression. To
    set the regular expression use the setRegExp() slot, use the
    regExp() function to retrieve the currently set expression.

    In addition, QtStringPropertyManager provides the valueChanged() signal
    which is emitted whenever a property created by this manager
    changes, and the regExpChanged() signal which is emitted whenever
    such a property changes its currently set regular expression.

    \sa QtAbstractPropertyManager, QtLineEditFactory
*/

/*!
    \fn void QtStringPropertyManager::valueChanged(QtProperty *property, const QString &value)

    This signal is emitted whenever a property created by this manager
    changes its value, passing a pointer to the \a property and the
    new \a value as parameters.

    \sa setValue()
*/

/*!
    \fn void QtStringPropertyManager::regExpChanged(QtProperty *property, const QRegExp &regExp)

    This signal is emitted whenever a property created by this manager
    changes its currenlty set regular expression, passing a pointer to
    the \a property and the new \a regExp as parameters.

    \sa setRegExp()
*/

/*!
    Creates a manager with the given \a parent.
*/
QtStringPropertyManager::QtStringPropertyManager(QObject *parent)
    : QtAbstractPropertyManager(parent)
{
    d_ptr = new QtStringPropertyManagerPrivate;
    d_ptr->q_ptr = this;
}

/*!
    Destroys this manager, and all the properties it has created.
*/
QtStringPropertyManager::~QtStringPropertyManager()
{
    clear();
    delete d_ptr;
}

/*!
    Returns the given \a property's value.

    If the given property is not managed by this manager, this
    function returns an empty string.

    \sa setValue()
*/
QString QtStringPropertyManager::value(const QtProperty *property) const
{
    return getValue<QString>(d_ptr->m_values, property);
}


QVariant QtStringPropertyManager::variantValue(const QtProperty *property) const
{
	return QVariant::fromValue(value(property));
}


QVariant QtStringPropertyManager::attributeValue(const QtProperty *property, const QString &attribute) const
{
	if( attribute == "regExp" ) {
		return QVariant(regExp(property));
	} else if( attribute == "echoMode" ) {
		return QVariant(echoMode(property));
	} else if( attribute == "readOnly" ) {
		return QVariant(isReadOnly(property));
	} else {
		qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	}
	return QVariant();

}


int QtStringPropertyManager::propertyTypeId() const
{
	return QVariant::String;
}


/*!
    Returns the given \a property's currently set regular expression.

    If the given \a property is not managed by this manager, this
    function returns an empty expression.

    \sa setRegExp()
*/
QRegExp QtStringPropertyManager::regExp(const QtProperty *property) const
{
    return getData<QRegExp>(d_ptr->m_values, &QtStringPropertyManagerPrivate::Data::regExp, property, QRegExp());
}

/*!
    \reimp
*/
EchoMode QtStringPropertyManager::echoMode(const QtProperty *property) const
{
    return (EchoMode)getData<int>(d_ptr->m_values, &QtStringPropertyManagerPrivate::Data::echoMode, property, 0);
}

/*!
    Returns read-only status of the property.

    When property is read-only it's value can be selected and copied from editor but not modified.

    \sa QtStringPropertyManager::setReadOnly
*/
bool QtStringPropertyManager::isReadOnly(const QtProperty *property) const
{
    return getData<bool>(d_ptr->m_values, &QtStringPropertyManagerPrivate::Data::readOnly, property, false);
}

/*!
    \reimp
*/
QString QtStringPropertyManager::valueText(const QtProperty *property) const
{
    const QtStringPropertyManagerPrivate::PropertyValueMap::const_iterator it = d_ptr->m_values.constFind(property);
    if (it == d_ptr->m_values.constEnd())
        return QString();

    return it.value().val;
}

/*!
    \reimp
*/
QString QtStringPropertyManager::displayText(const QtProperty *property) const
{
    const QtStringPropertyManagerPrivate::PropertyValueMap::const_iterator it = d_ptr->m_values.constFind(property);
    if (it == d_ptr->m_values.constEnd())
        return QString();

    QLineEdit edit;
    edit.setEchoMode((EchoMode)it.value().echoMode);
    edit.setText(it.value().val);
    return edit.displayText();
}

/*!
    \fn void QtStringPropertyManager::setValue(QtProperty *property, const QString &value)

    Sets the value of the given \a property to \a value.

    If the specified \a value doesn't match the given \a property's
    regular expression, this function does nothing.

    \sa value(), setRegExp(), valueChanged()
*/
void QtStringPropertyManager::setValue(QtProperty *property, const QString &val)
{
    const QtStringPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    QtStringPropertyManagerPrivate::Data data = it.value();

    if (data.val == val)
        return;

    if (data.regExp.isValid() && !data.regExp.exactMatch(val))
        return;

    data.val = val;

    it.value() = data;

    emit propertyChanged(property);
    emit valueChanged(property, data.val);
	emit QtAbstractPropertyManager::valueChanged(property, QVariant(data.val));
}


void QtStringPropertyManager::setValue(QtProperty *property, const QVariant &val)
{
	Q_ASSERT(val.canConvert<QString>());
	auto s_val = qvariant_cast<QString>(val);
	setValue(property, s_val);
}


void QtStringPropertyManager::setAttribute(QtProperty *property, const QString &attribute, const QVariant &value)
{
	if( attribute == "regExp" ) {
		setRegExp(property, qvariant_cast<QRegExp>(value));
	} else if( attribute == "echoMode" ) {
		setEchoMode(property, (EchoMode)qvariant_cast<int>(value));
	} else if( attribute == "readOnly" ) {
		setReadOnly(property, qvariant_cast<bool>(value));
	} else {
		qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	}
}


/*!
    Sets the regular expression of the given \a property to \a regExp.

    \sa regExp(), setValue(), regExpChanged()
*/
void QtStringPropertyManager::setRegExp(QtProperty *property, const QRegExp &regExp)
{
    const QtStringPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    QtStringPropertyManagerPrivate::Data data = it.value() ;

    if (data.regExp == regExp)
        return;

    data.regExp = regExp;

    it.value() = data;

    emit regExpChanged(property, data.regExp);
	emit attributeChanged(property, "regExp", data.regExp);
}


void QtStringPropertyManager::setEchoMode(QtProperty *property, EchoMode echoMode)
{
    const QtStringPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    QtStringPropertyManagerPrivate::Data data = it.value();

    if (data.echoMode == echoMode)
        return;

    data.echoMode = echoMode;
    it.value() = data;

    emit propertyChanged(property);
    emit echoModeChanged(property, data.echoMode);
	emit attributeChanged(property, "echoMode", (int)data.echoMode);
}

/*!
    Sets read-only status of the property.

    \sa QtStringPropertyManager::setReadOnly
*/
void QtStringPropertyManager::setReadOnly(QtProperty *property, bool readOnly)
{
    const QtStringPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    QtStringPropertyManagerPrivate::Data data = it.value();

    if (data.readOnly == readOnly)
        return;

    data.readOnly = readOnly;
    it.value() = data;

    emit propertyChanged(property);
    emit readOnlyChanged(property, data.readOnly);
	emit attributeChanged(property, "readOnly", data.readOnly);
}

/*!
    \reimp
*/
void QtStringPropertyManager::initializeProperty(QtProperty *property)
{
    d_ptr->m_values[property] = QtStringPropertyManagerPrivate::Data();
}

/*!
    \reimp
*/
void QtStringPropertyManager::uninitializeProperty(QtProperty *property)
{
    d_ptr->m_values.remove(property);
}

// QtBoolPropertyManager
//     Return an icon containing a check box indicator
static QIcon drawCheckBox(bool value)
{
    QStyleOptionButton opt;
    opt.state |= value ? QStyle::State_On : QStyle::State_Off;
    opt.state |= QStyle::State_Enabled;
    const QStyle *style = QApplication::style();
    // Figure out size of an indicator and make sure it is not scaled down in a list view item
    // by making the pixmap as big as a list view icon and centering the indicator in it.
    // (if it is smaller, it can't be helped)
    const int indicatorWidth = style->pixelMetric(QStyle::PM_IndicatorWidth, &opt);
    const int indicatorHeight = style->pixelMetric(QStyle::PM_IndicatorHeight, &opt);
    const int listViewIconSize = indicatorWidth;
    const int pixmapWidth = indicatorWidth;
    const int pixmapHeight = qMax(indicatorHeight, listViewIconSize);

    opt.rect = QRect(0, 0, indicatorWidth, indicatorHeight);
    QPixmap pixmap = QPixmap(pixmapWidth, pixmapHeight);
    pixmap.fill(Qt::transparent);
    {
        // Center?
        const int xoff = (pixmapWidth  > indicatorWidth)  ? (pixmapWidth  - indicatorWidth)  / 2 : 0;
        const int yoff = (pixmapHeight > indicatorHeight) ? (pixmapHeight - indicatorHeight) / 2 : 0;
        QPainter painter(&pixmap);
        painter.translate(xoff, yoff);
        style->drawPrimitive(QStyle::PE_IndicatorCheckBox, &opt, &painter);
    }
    return QIcon(pixmap);
}


QtBoolPropertyManagerPrivate::QtBoolPropertyManagerPrivate() :
    m_checkedIcon(drawCheckBox(true)),
    m_uncheckedIcon(drawCheckBox(false))
{
}

/*!
    \class QtBoolPropertyManager

    \brief The QtBoolPropertyManager class provides and manages boolean properties.

    The property's value can be retrieved using the value() function,
    and set using the setValue() slot.

    In addition, QtBoolPropertyManager provides the valueChanged() signal
    which is emitted whenever a property created by this manager
    changes.

    \sa QtAbstractPropertyManager, QtCheckBoxFactory
*/

/*!
    \fn void QtBoolPropertyManager::valueChanged(QtProperty *property, bool value)

    This signal is emitted whenever a property created by this manager
    changes its value, passing a pointer to the \a property and the
    new \a value as parameters.
*/

/*!
    Creates a manager with the given \a parent.
*/
QtBoolPropertyManager::QtBoolPropertyManager(QObject *parent)
    : QtAbstractPropertyManager(parent)
{
    d_ptr = new QtBoolPropertyManagerPrivate;
    d_ptr->q_ptr = this;
}

/*!
    Destroys this manager, and all the properties it has created.
*/
QtBoolPropertyManager::~QtBoolPropertyManager()
{
    clear();
    delete d_ptr;
}

/*!
    Returns the given \a property's value.

    If the given \a property is not managed by \e this manager, this
    function returns false.

    \sa setValue()
*/
bool QtBoolPropertyManager::value(const QtProperty *property) const
{
    return getValue<bool>(d_ptr->m_values, property, false);
}


QVariant QtBoolPropertyManager::variantValue(const QtProperty *property) const
{
	return QVariant::fromValue(value(property));
}


QVariant QtBoolPropertyManager::attributeValue(const QtProperty *property, const QString &attribute) const
{
	if( attribute == "textVisible" ) {
		return QVariant(textVisible(property));
	} else {
		qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	}
	return QVariant();
}


int QtBoolPropertyManager::propertyTypeId() const
{
	return QVariant::Bool;
}


bool QtBoolPropertyManager::textVisible(const QtProperty *property) const
{
    return getData<bool>(d_ptr->m_values, &QtBoolPropertyManagerPrivate::Data::textVisible, property, false);
}

/*!
    \reimp
*/
QString QtBoolPropertyManager::valueText(const QtProperty *property) const
{
    const QtBoolPropertyManagerPrivate::PropertyValueMap::const_iterator it = d_ptr->m_values.constFind(property);
    if (it == d_ptr->m_values.constEnd())
        return QString();

    const QtBoolPropertyManagerPrivate::Data &data = it.value();
    if (!data.textVisible)
        return QString();

    static const QString trueText = tr("True");
    static const QString falseText = tr("False");
    return data.val ? trueText : falseText;
}

/*!
    \reimp
*/
QIcon QtBoolPropertyManager::valueIcon(const QtProperty *property) const
{
    const QtBoolPropertyManagerPrivate::PropertyValueMap::const_iterator it = d_ptr->m_values.constFind(property);
    if (it == d_ptr->m_values.constEnd())
        return QIcon();

    return it.value().val ? d_ptr->m_checkedIcon : d_ptr->m_uncheckedIcon;
}

/*!
    \fn void QtBoolPropertyManager::setValue(QtProperty *property, bool value)

    Sets the value of the given \a property to \a value.

    \sa value()
*/
void QtBoolPropertyManager::setValue(QtProperty *property, bool val)
{
    const QtBoolPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    QtBoolPropertyManagerPrivate::Data data = it.value();

    if (data.val == val)
        return;

    data.val = val;
    it.value() = data;

    emit propertyChanged(property);
    emit valueChanged(property, data.val);
	emit QtAbstractPropertyManager::valueChanged(property, QVariant(data.val));
}


void QtBoolPropertyManager::setValue(QtProperty *property, const QVariant &val)
{
	Q_ASSERT(val.canConvert<bool>());
	auto s_val = qvariant_cast<bool>(val);
	setValue(property, s_val);
}

void QtBoolPropertyManager::setAttribute(QtProperty *property, const QString &attribute, const QVariant &value)
{
	if( attribute == "textVisible" ) {
		setTextVisible(property, qvariant_cast<bool>(value));
	} else {
		qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	}
}



void QtBoolPropertyManager::setTextVisible(QtProperty *property, bool textVisible)
{
    const QtBoolPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    QtBoolPropertyManagerPrivate::Data data = it.value();

    if (data.textVisible == textVisible)
        return;

    data.textVisible = textVisible;
    it.value() = data;

    emit propertyChanged(property);
    emit textVisibleChanged(property, data.textVisible);
    emit attributeChanged(property, "textVisible", data.textVisible);
}

/*!
    \reimp
*/
void QtBoolPropertyManager::initializeProperty(QtProperty *property)
{
    d_ptr->m_values[property] = QtBoolPropertyManagerPrivate::Data();
}

/*!
    \reimp
*/
void QtBoolPropertyManager::uninitializeProperty(QtProperty *property)
{
    d_ptr->m_values.remove(property);
}

// QtDatePropertyManager


/*!
    \class QtDatePropertyManager

    \brief The QtDatePropertyManager provides and manages QDate properties.

    A date property has a current value, and a range specifying the
    valid dates. The range is defined by a minimum and a maximum
    value.

    The property's values can be retrieved using the minimum(),
    maximum() and value() functions, and can be set using the
    setMinimum(), setMaximum() and setValue() slots. Alternatively,
    the range can be defined in one go using the setRange() slot.

    In addition, QtDatePropertyManager provides the valueChanged() signal
    which is emitted whenever a property created by this manager
    changes, and the rangeChanged() signal which is emitted whenever
    such a property changes its range of valid dates.

    \sa QtAbstractPropertyManager, QtDateEditFactory, QtDateTimePropertyManager
*/

/*!
    \fn void QtDatePropertyManager::valueChanged(QtProperty *property, const QDate &value)

    This signal is emitted whenever a property created by this manager
    changes its value, passing a pointer to the \a property and the new
    \a value as parameters.

    \sa setValue()
*/

/*!
    \fn void QtDatePropertyManager::rangeChanged(QtProperty *property, const QDate &minimum, const QDate &maximum)

    This signal is emitted whenever a property created by this manager
    changes its range of valid dates, passing a pointer to the \a
    property and the new \a minimum and \a maximum dates.

    \sa setRange()
*/

/*!
    Creates a manager with the given \a parent.
*/
QtDatePropertyManager::QtDatePropertyManager(QObject *parent)
    : QtAbstractPropertyManager(parent)
{
    d_ptr = new QtDatePropertyManagerPrivate;
    d_ptr->q_ptr = this;

    QLocale loc;
    d_ptr->m_format = loc.dateFormat(QLocale::ShortFormat);
}

/*!
    Destroys this manager, and all the properties it has created.
*/
QtDatePropertyManager::~QtDatePropertyManager()
{
    clear();
    delete d_ptr;
}

/*!
    Returns the given \a property's value.

    If the given \a property is not managed by \e this manager, this
    function returns an invalid date.

    \sa setValue()
*/
QDate QtDatePropertyManager::value(const QtProperty *property) const
{
    return getValue<QDate>(d_ptr->m_values, property);
}


QVariant QtDatePropertyManager::variantValue(const QtProperty *property) const
{
	return QVariant::fromValue(value(property));
}


QVariant QtDatePropertyManager::attributeValue(const QtProperty *property, const QString &attribute) const
{
	if( attribute == "minimum" ) {
		return QVariant(minimum(property));
	} else if( attribute == "maximum" ) {
		return QVariant(maximum(property));
	} else {
		qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	}
	return QVariant();
}


int QtDatePropertyManager::propertyTypeId() const
{
	return QVariant::Date;
}


/*!
    Returns the given \a  property's  minimum date.

    \sa maximum(), setRange()
*/
QDate QtDatePropertyManager::minimum(const QtProperty *property) const
{
    return getMinimum<QDate>(d_ptr->m_values, property);
}

/*!
    Returns the given \a property's maximum date.

    \sa minimum(), setRange()
*/
QDate QtDatePropertyManager::maximum(const QtProperty *property) const
{
    return getMaximum<QDate>(d_ptr->m_values, property);
}

/*!
    \reimp
*/
QString QtDatePropertyManager::valueText(const QtProperty *property) const
{
    const QtDatePropertyManagerPrivate::PropertyValueMap::const_iterator it = d_ptr->m_values.constFind(property);
    if (it == d_ptr->m_values.constEnd())
        return QString();
    return it.value().val.toString(d_ptr->m_format);
}

/*!
    \fn void QtDatePropertyManager::setValue(QtProperty *property, const QDate &value)

    Sets the value of the given \a property to \a value.

    If the specified \a value is not a valid date according to the
    given \a property's range, the value is adjusted to the nearest
    valid value within the range.

    \sa value(), setRange(), valueChanged()
*/
void QtDatePropertyManager::setValue(QtProperty *property, const QDate &val)
{
    void (QtDatePropertyManagerPrivate::*setSubPropertyValue)(QtProperty *, const QDate &) = 0;
    setValueInRange<const QDate &, QtDatePropertyManagerPrivate, QtDatePropertyManager, const QDate>(this, d_ptr,
                &QtDatePropertyManager::propertyChanged,
                &QtDatePropertyManager::valueChanged,
                &QtAbstractPropertyManager::valueChanged,
                property, val, setSubPropertyValue);
}


void QtDatePropertyManager::setValue(QtProperty *property, const QVariant &val)
{
	Q_ASSERT(val.canConvert<QDate>());
	auto s_val = qvariant_cast<QDate>(val);
	setValue(property, s_val);
}


void QtDatePropertyManager::setAttribute(QtProperty *property, const QString &attribute, const QVariant &value)
{
	if( attribute == "minimum" ) {
		setMinimum(property, qvariant_cast<QDate>(value));
	} else if( attribute == "maximum" ) {
		setMaximum(property, qvariant_cast<QDate>(value));
	} else {
		qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	}
}


/*!
    Sets the minimum value for the given \a property to \a minVal.

    When setting the minimum value, the maximum and current values are
    adjusted if necessary (ensuring that the range remains valid and
    that the current value is within in the range).

    \sa minimum(), setRange()
*/
void QtDatePropertyManager::setMinimum(QtProperty *property, const QDate &minVal)
{
	auto it = d_ptr->m_values.find(property);
	if( it == d_ptr->m_values.end() ) {
		return;
	}

	auto& data = it.value();

	if( data.minimumValue() == minVal ) {
		return;
	}

	const auto oldMaxVal = data.maxVal;
	const auto oldVal = data.val;

	data.setMinimumValue(minVal);

	emit rangeChanged(property, data.minVal, data.maxVal);
	emit attributeChanged(property, "minimum", data.minVal);

	if( oldMaxVal != data.maxVal ) {
		emit attributeChanged(property, "maximum", data.maxVal);
	}

	if( data.val != oldVal ) {
		emit propertyChanged(property);
		emit valueChanged(property, data.val);
		emit QtAbstractPropertyManager::valueChanged(property, data.val);
	}
}

/*!
    Sets the maximum value for the given \a property to \a maxVal.

    When setting the maximum value, the minimum and current
    values are adjusted if necessary (ensuring that the range remains
    valid and that the current value is within in the range).

    \sa maximum(), setRange()
*/
void QtDatePropertyManager::setMaximum(QtProperty *property, const QDate &maxVal)
{
	auto it = d_ptr->m_values.find(property);
	if( it == d_ptr->m_values.end() ) {
		return;
	}

	auto& data = it.value();

	if( data.maximumValue() == maxVal ) {
		return;
	}

	const auto oldMinVal = data.minVal;
	const auto oldVal = data.val;

	data.setMaximumValue(maxVal);

	emit rangeChanged(property, data.minVal, data.maxVal);
	emit attributeChanged(property, "maximum", data.maxVal);

	if( oldMinVal != data.minVal ) {
		emit attributeChanged(property, "minimum", data.minVal);
	}

	if( data.val != oldVal ) {
		emit propertyChanged(property);
		emit valueChanged(property, data.val);
		emit QtAbstractPropertyManager::valueChanged(property, data.val);
	}
}

/*!
    \fn void QtDatePropertyManager::setRange(QtProperty *property, const QDate &minimum, const QDate &maximum)

    Sets the range of valid dates.

    This is a convenience function defining the range of valid dates
    in one go; setting the \a minimum and \a maximum values for the
    given \a property with a single function call.

    When setting a new date range, the current value is adjusted if
    necessary (ensuring that the value remains in date range).

    \sa setMinimum(), setMaximum(), rangeChanged()
*/
void QtDatePropertyManager::setRange(QtProperty *property, const QDate &_minVal, const QDate &_maxVal)
{
	auto it = d_ptr->m_values.find(property);
	if( it == d_ptr->m_values.end() ) {
		return;
	}

	auto minVal = _minVal;
	auto maxVal = _maxVal;

	orderBorders(minVal, maxVal);

	auto& data = it.value();

	if( data.minimumValue() == minVal && data.maximumValue() == maxVal ) {
		return;
	}

	const auto oldMinVal = data.minVal;
	const auto oldMaxVal = data.maxVal;
	const auto oldVal = data.val;

	data.setMinimumValue(minVal);
	data.setMaximumValue(maxVal);

	emit rangeChanged(property, data.minVal, data.maxVal);

	if( oldMinVal != data.minVal ) {
		emit attributeChanged(property, "minimum", data.minVal);
	}
	if( oldMaxVal != data.maxVal ) {
		emit attributeChanged(property, "maximum", data.maxVal);
	}
	if( data.val != oldVal ) {
		emit propertyChanged(property);
		emit valueChanged(property, data.val);
		emit QtAbstractPropertyManager::valueChanged(property, data.val);
	}
}

/*!
    \reimp
*/
void QtDatePropertyManager::initializeProperty(QtProperty *property)
{
    d_ptr->m_values[property] = QtDatePropertyManagerPrivate::Data();
}

/*!
    \reimp
*/
void QtDatePropertyManager::uninitializeProperty(QtProperty *property)
{
    d_ptr->m_values.remove(property);
}

// QtTimePropertyManager

/*!
    \class QtTimePropertyManager

    \brief The QtTimePropertyManager provides and manages QTime properties.

    A time property's value can be retrieved using the value()
    function, and set using the setValue() slot.

    In addition, QtTimePropertyManager provides the valueChanged() signal
    which is emitted whenever a property created by this manager
    changes.

    \sa QtAbstractPropertyManager, QtTimeEditFactory
*/

/*!
    \fn void QtTimePropertyManager::valueChanged(QtProperty *property, const QTime &value)

    This signal is emitted whenever a property created by this manager
    changes its value, passing a pointer to the \a property and the
    new \a value as parameters.

    \sa setValue()
*/

/*!
    Creates a manager with the given \a parent.
*/
QtTimePropertyManager::QtTimePropertyManager(QObject *parent)
    : QtAbstractPropertyManager(parent)
{
    d_ptr = new QtTimePropertyManagerPrivate;
    d_ptr->q_ptr = this;

    QLocale loc;
    d_ptr->m_format = loc.timeFormat(QLocale::ShortFormat);
}

/*!
    Destroys this manager, and all the properties it has created.
*/
QtTimePropertyManager::~QtTimePropertyManager()
{
    clear();
    delete d_ptr;
}

/*!
    Returns the given \a property's value.

    If the given property is not managed by this manager, this
    function returns an invalid time object.

    \sa setValue()
*/
QTime QtTimePropertyManager::value(const QtProperty *property) const
{
    return d_ptr->m_values.value(property, QTime());
}


QVariant QtTimePropertyManager::variantValue(const QtProperty *property) const
{
	return QVariant::fromValue(value(property));
}


QVariant QtTimePropertyManager::attributeValue(const QtProperty *property, const QString &attribute) const
{
	Q_UNUSED(property);
	qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	return QVariant();
}



int QtTimePropertyManager::propertyTypeId() const
{
	return QVariant::Time;
}


/*!
    \reimp
*/
QString QtTimePropertyManager::valueText(const QtProperty *property) const
{
   const QtTimePropertyManagerPrivate::PropertyValueMap::const_iterator it = d_ptr->m_values.constFind(property);
    if (it == d_ptr->m_values.constEnd())
        return QString();
    return it.value().toString(d_ptr->m_format);
}

/*!
    \fn void QtTimePropertyManager::setValue(QtProperty *property, const QTime &value)

    Sets the value of the given \a property to \a value.

    \sa value(), valueChanged()
*/
void QtTimePropertyManager::setValue(QtProperty *property, const QTime &val)
{
    setSimpleValue<const QTime &, QTime, QtTimePropertyManager>(d_ptr->m_values, this,
                &QtTimePropertyManager::propertyChanged,
                &QtTimePropertyManager::valueChanged,
                &QtAbstractPropertyManager::valueChanged,
                property, val);
}


void QtTimePropertyManager::setValue(QtProperty *property, const QVariant &val)
{
	Q_ASSERT(val.canConvert<QTime>());
	auto s_val = qvariant_cast<QTime>(val);
	setValue(property, s_val);
}


void QtTimePropertyManager::setAttribute(QtProperty *property, const QString &attribute, const QVariant &value)
{
	Q_UNUSED(property); Q_UNUSED(value);
	qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
}


/*!
    \reimp
*/
void QtTimePropertyManager::initializeProperty(QtProperty *property)
{
    d_ptr->m_values[property] = QTime::currentTime();
}

/*!
    \reimp
*/
void QtTimePropertyManager::uninitializeProperty(QtProperty *property)
{
    d_ptr->m_values.remove(property);
}

// QtDateTimePropertyManager

/*! \class QtDateTimePropertyManager

    \brief The QtDateTimePropertyManager provides and manages QDateTime properties.

    A date and time property has a current value which can be
    retrieved using the value() function, and set using the setValue()
    slot. In addition, QtDateTimePropertyManager provides the
    valueChanged() signal which is emitted whenever a property created
    by this manager changes.

    \sa QtAbstractPropertyManager, QtDateTimeEditFactory, QtDatePropertyManager
*/

/*!
    \fn void QtDateTimePropertyManager::valueChanged(QtProperty *property, const QDateTime &value)

    This signal is emitted whenever a property created by this manager
    changes its value, passing a pointer to the \a property and the new
    \a value as parameters.
*/

/*!
    Creates a manager with the given \a parent.
*/
QtDateTimePropertyManager::QtDateTimePropertyManager(QObject *parent)
    : QtAbstractPropertyManager(parent)
{
    d_ptr = new QtDateTimePropertyManagerPrivate;
    d_ptr->q_ptr = this;

    QLocale loc;
    d_ptr->m_format = loc.dateFormat(QLocale::ShortFormat);
    d_ptr->m_format += QLatin1Char(' ');
    d_ptr->m_format += loc.timeFormat(QLocale::ShortFormat);
}

/*!
    Destroys this manager, and all the properties it has created.
*/
QtDateTimePropertyManager::~QtDateTimePropertyManager()
{
    clear();
    delete d_ptr;
}

/*!
    Returns the given \a property's value.

    If the given \a property is not managed by this manager, this
    function returns an invalid QDateTime object.

    \sa setValue()
*/
QDateTime QtDateTimePropertyManager::value(const QtProperty *property) const
{
    return d_ptr->m_values.value(property, QDateTime());
}


QVariant QtDateTimePropertyManager::attributeValue(const QtProperty *property, const QString &attribute) const
{
	Q_UNUSED(property);
	qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	return QVariant();
}


int QtDateTimePropertyManager::propertyTypeId() const
{
	return QVariant::DateTime;
}


QVariant QtDateTimePropertyManager::variantValue(const QtProperty *property) const
{
	return QVariant::fromValue(value(property));
}


/*!
    \reimp
*/
QString QtDateTimePropertyManager::valueText(const QtProperty *property) const
{
   const QtDateTimePropertyManagerPrivate::PropertyValueMap::const_iterator it = d_ptr->m_values.constFind(property);
    if (it == d_ptr->m_values.constEnd())
        return QString();
    return it.value().toString(d_ptr->m_format);
}

/*!
    \fn void QtDateTimePropertyManager::setValue(QtProperty *property, const QDateTime &value)

    Sets the value of the given \a property to \a value.

    \sa value(), valueChanged()
*/
void QtDateTimePropertyManager::setValue(QtProperty *property, const QDateTime &val)
{
    setSimpleValue<const QDateTime &, QDateTime, QtDateTimePropertyManager>(d_ptr->m_values, this,
                &QtDateTimePropertyManager::propertyChanged,
                &QtDateTimePropertyManager::valueChanged,
                &QtAbstractPropertyManager::valueChanged,
                property, val);
}


void QtDateTimePropertyManager::setValue(QtProperty *property, const QVariant &val)
{
	Q_ASSERT(val.canConvert<QDateTime>());
	auto s_val = qvariant_cast<QDateTime>(val);
	setValue(property, s_val);
}


void QtDateTimePropertyManager::setAttribute(QtProperty *property, const QString &attribute, const QVariant &value)
{
	Q_UNUSED(property); Q_UNUSED(value);
	qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
}


/*!
    \reimp
*/
void QtDateTimePropertyManager::initializeProperty(QtProperty *property)
{
    d_ptr->m_values[property] = QDateTime::currentDateTime();
}

/*!
    \reimp
*/
void QtDateTimePropertyManager::uninitializeProperty(QtProperty *property)
{
    d_ptr->m_values.remove(property);
}

// QtKeySequencePropertyManager

/*! \class QtKeySequencePropertyManager

    \brief The QtKeySequencePropertyManager provides and manages QKeySequence properties.

    A key sequence's value can be retrieved using the value()
    function, and set using the setValue() slot.

    In addition, QtKeySequencePropertyManager provides the valueChanged() signal
    which is emitted whenever a property created by this manager
    changes.

    \sa QtAbstractPropertyManager
*/

/*!
    \fn void QtKeySequencePropertyManager::valueChanged(QtProperty *property, const QKeySequence &value)

    This signal is emitted whenever a property created by this manager
    changes its value, passing a pointer to the \a property and the new
    \a value as parameters.
*/

/*!
    Creates a manager with the given \a parent.
*/
QtKeySequencePropertyManager::QtKeySequencePropertyManager(QObject *parent)
    : QtAbstractPropertyManager(parent)
{
    d_ptr = new QtKeySequencePropertyManagerPrivate;
    d_ptr->q_ptr = this;
}

/*!
    Destroys this manager, and all the properties it has created.
*/
QtKeySequencePropertyManager::~QtKeySequencePropertyManager()
{
    clear();
    delete d_ptr;
}

/*!
    Returns the given \a property's value.

    If the given \a property is not managed by this manager, this
    function returns an empty QKeySequence object.

    \sa setValue()
*/
QKeySequence QtKeySequencePropertyManager::value(const QtProperty *property) const
{
    return d_ptr->m_values.value(property, QKeySequence());
}


QVariant QtKeySequencePropertyManager::attributeValue(const QtProperty *property, const QString &attribute) const
{
	Q_UNUSED(property);
	qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	return QVariant();
}


int QtKeySequencePropertyManager::propertyTypeId() const
{
	return QVariant::KeySequence;
}


QVariant QtKeySequencePropertyManager::variantValue(const QtProperty *property) const
{
	return QVariant::fromValue(value(property));
}


/*!
    \reimp
*/
QString QtKeySequencePropertyManager::valueText(const QtProperty *property) const
{
    const QtKeySequencePropertyManagerPrivate::PropertyValueMap::const_iterator it = d_ptr->m_values.constFind(property);
    if (it == d_ptr->m_values.constEnd())
        return QString();
    return it.value().toString(QKeySequence::NativeText);
}

/*!
    \fn void QtKeySequencePropertyManager::setValue(QtProperty *property, const QKeySequence &value)

    Sets the value of the given \a property to \a value.

    \sa value(), valueChanged()
*/
void QtKeySequencePropertyManager::setValue(QtProperty *property, const QKeySequence &val)
{
    setSimpleValue<const QKeySequence &, QKeySequence, QtKeySequencePropertyManager>(d_ptr->m_values, this,
                &QtKeySequencePropertyManager::propertyChanged,
                &QtKeySequencePropertyManager::valueChanged,
                &QtAbstractPropertyManager::valueChanged,
                property, val);
}


void QtKeySequencePropertyManager::setValue(QtProperty *property, const QVariant &val)
{
	Q_ASSERT(val.canConvert<QKeySequence>());
	auto s_val = qvariant_cast<QKeySequence>(val);
	setValue(property, s_val);
}


void QtKeySequencePropertyManager::setAttribute(QtProperty *property, const QString &attribute, const QVariant &value)
{
	Q_UNUSED(property); Q_UNUSED(value);
	qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
}


/*!
    \reimp
*/
void QtKeySequencePropertyManager::initializeProperty(QtProperty *property)
{
    d_ptr->m_values[property] = QKeySequence();
}

/*!
    \reimp
*/
void QtKeySequencePropertyManager::uninitializeProperty(QtProperty *property)
{
    d_ptr->m_values.remove(property);
}

// QtCharPropertyManager

/*! \class QtCharPropertyManager

    \brief The QtCharPropertyManager provides and manages QChar properties.

    A char's value can be retrieved using the value()
    function, and set using the setValue() slot.

    In addition, QtCharPropertyManager provides the valueChanged() signal
    which is emitted whenever a property created by this manager
    changes.

    \sa QtAbstractPropertyManager
*/

/*!
    \fn void QtCharPropertyManager::valueChanged(QtProperty *property, const QChar &value)

    This signal is emitted whenever a property created by this manager
    changes its value, passing a pointer to the \a property and the new
    \a value as parameters.
*/

/*!
    Creates a manager with the given \a parent.
*/
QtCharPropertyManager::QtCharPropertyManager(QObject *parent)
    : QtAbstractPropertyManager(parent)
{
    d_ptr = new QtCharPropertyManagerPrivate;
    d_ptr->q_ptr = this;
}

/*!
    Destroys this manager, and all the properties it has created.
*/
QtCharPropertyManager::~QtCharPropertyManager()
{
    clear();
    delete d_ptr;
}

/*!
    Returns the given \a property's value.

    If the given \a property is not managed by this manager, this
    function returns an null QChar object.

    \sa setValue()
*/
QChar QtCharPropertyManager::value(const QtProperty *property) const
{
    return d_ptr->m_values.value(property, QChar());
}


QVariant QtCharPropertyManager::attributeValue(const QtProperty *property, const QString &attribute) const
{
	Q_UNUSED(property);
	qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	return QVariant();
}


int QtCharPropertyManager::propertyTypeId() const
{
	return QVariant::Char;
}


QVariant QtCharPropertyManager::variantValue(const QtProperty *property) const
{
	return QVariant::fromValue(value(property));
}


/*!
    \reimp
*/
QString QtCharPropertyManager::valueText(const QtProperty *property) const
{
   const QtCharPropertyManagerPrivate::PropertyValueMap::const_iterator it = d_ptr->m_values.constFind(property);
    if (it == d_ptr->m_values.constEnd())
        return QString();
    const QChar c = it.value();
    return c.isNull() ? QString() : QString(c);
}

/*!
    \fn void QtCharPropertyManager::setValue(QtProperty *property, const QChar &value)

    Sets the value of the given \a property to \a value.

    \sa value(), valueChanged()
*/
void QtCharPropertyManager::setValue(QtProperty *property, const QChar &val)
{
    setSimpleValue<const QChar &, QChar, QtCharPropertyManager>(d_ptr->m_values, this,
                &QtCharPropertyManager::propertyChanged,
                &QtCharPropertyManager::valueChanged,
                &QtAbstractPropertyManager::valueChanged,
                property, val);
}


void QtCharPropertyManager::setValue(QtProperty *property, const QVariant &val)
{
	Q_ASSERT(val.canConvert<QChar>());
	auto s_val = qvariant_cast<QChar>(val);
	setValue(property, s_val);
}


void QtCharPropertyManager::setAttribute(QtProperty *property, const QString &attribute, const QVariant &value)
{
	Q_UNUSED(property); Q_UNUSED(value);
	qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
}


/*!
    \reimp
*/
void QtCharPropertyManager::initializeProperty(QtProperty *property)
{
    d_ptr->m_values[property] = QChar();
}

/*!
    \reimp
*/
void QtCharPropertyManager::uninitializeProperty(QtProperty *property)
{
    d_ptr->m_values.remove(property);
}

// QtLocalePropertyManager

QtLocalePropertyManagerPrivate::QtLocalePropertyManagerPrivate()
{
}

void QtLocalePropertyManagerPrivate::slotEnumChanged(QtProperty *property, int value)
{
    if (QtProperty *prop = m_languageToProperty.value(property, 0)) {
        const QLocale loc = m_values[prop];
        QLocale::Language newLanguage = loc.language();
        QLocale::Country newCountry = loc.country();
        metaEnumProvider()->indexToLocale(value, 0, &newLanguage, 0);
        QLocale newLoc(newLanguage, newCountry);
        q_ptr->setValue(prop, newLoc);
    } else if (QtProperty *prop = m_countryToProperty.value(property, 0)) {
        const QLocale loc = m_values[prop];
        QLocale::Language newLanguage = loc.language();
        QLocale::Country newCountry = loc.country();
        metaEnumProvider()->indexToLocale(m_enumPropertyManager->value(m_propertyToLanguage.value(prop)), value, &newLanguage, &newCountry);
        QLocale newLoc(newLanguage, newCountry);
        q_ptr->setValue(prop, newLoc);
    }
}

void QtLocalePropertyManagerPrivate::slotPropertyDestroyed(QtProperty *property)
{
    if (QtProperty *subProp = m_languageToProperty.value(property, 0)) {
        m_propertyToLanguage[subProp] = 0;
        m_languageToProperty.remove(property);
    } else if (QtProperty *subProp = m_countryToProperty.value(property, 0)) {
        m_propertyToCountry[subProp] = 0;
        m_countryToProperty.remove(property);
    }
}

/*!
    \class QtLocalePropertyManager

    \brief The QtLocalePropertyManager provides and manages QLocale properties.

    A locale property has nested \e language and \e country
    subproperties. The top-level property's value can be retrieved
    using the value() function, and set using the setValue() slot.

    The subproperties are created by QtEnumPropertyManager object.
    These submanager can be retrieved using the subEnumPropertyManager()
    function. In order to provide editing widgets for the subproperties
    in a property browser widget, this manager must be associated with editor factory.

    In addition, QtLocalePropertyManager provides the valueChanged()
    signal which is emitted whenever a property created by this
    manager changes.

    \sa QtAbstractPropertyManager, QtEnumPropertyManager
*/

/*!
    \fn void QtLocalePropertyManager::valueChanged(QtProperty *property, const QLocale &value)

    This signal is emitted whenever a property created by this manager
    changes its value, passing a pointer to the \a property and the
    new \a value as parameters.

    \sa setValue()
*/

/*!
    Creates a manager with the given \a parent.
*/
QtLocalePropertyManager::QtLocalePropertyManager(QObject *parent)
    : QtAbstractPropertyManager(parent)
{
    d_ptr = new QtLocalePropertyManagerPrivate;
    d_ptr->q_ptr = this;

    d_ptr->m_enumPropertyManager = new QtEnumPropertyManager(this);
    connect(d_ptr->m_enumPropertyManager, SIGNAL(valueChanged(QtProperty *, int)),
                this, SLOT(slotEnumChanged(QtProperty *, int)));

    connect(d_ptr->m_enumPropertyManager, SIGNAL(propertyDestroyed(QtProperty *)),
                this, SLOT(slotPropertyDestroyed(QtProperty *)));
}

/*!
    Destroys this manager, and all the properties it has created.
*/
QtLocalePropertyManager::~QtLocalePropertyManager()
{
    clear();
    delete d_ptr;
}


// QtVector3DPropertyManager
void QtVector3DPropertyManagerPrivate::slotDoubleChanged(QtProperty *property, double value) {
    if (QtProperty *prop = m_xToProperty.value(property, 0)) {
        QVector3D v = m_values[prop].val;
        v.setX(value);
        q_ptr->setValue(prop, v);
    } else if (QtProperty *prop = m_yToProperty.value(property, 0)) {
        QVector3D v = m_values[prop].val;
        v.setY(value);
        q_ptr->setValue(prop, v);
    } else if (QtProperty *prop = m_zToProperty.value(property, 0)) {
        QVector3D v = m_values[prop].val;
        v.setZ(value);
        q_ptr->setValue(prop, v);
    }
}

void QtVector3DPropertyManagerPrivate::slotPropertyDestroyed(QtProperty *property) {
    if (QtProperty *pointProp  = m_xToProperty.value(property, 0)) {
        m_propertyToX[pointProp] = 0;
        m_xToProperty.remove(property);
    } else if (QtProperty *pointProp = m_yToProperty.value(property, 0)) {
        m_propertyToY[pointProp] = 0;
        m_yToProperty.remove(property);
    } else if (QtProperty *pointProp = m_zToProperty.value(property, 0)) {
        m_propertyToZ[pointProp] = 0;
        m_zToProperty.remove(property);
    }
}

/*! \class QtVector3DPropertyManager

    \brief The QtVector3DPropertyManager provides and manages QVector3D properties.			// TODO WRITE DOC

    A point property has nested \e x and \e y subproperties. The
    top-level property's value can be retrieved using the value()
    function, and set using the setValue() slot.

    The subproperties are created by a QtDoublePropertyManager object. This
    manager can be retrieved using the subDoublePropertyManager() function. In
    order to provide editing widgets for the subproperties in a
    property browser widget, this manager must be associated with an
    editor factory.

    In addition, QtPointFPropertyManager provides the valueChanged() signal which
    is emitted whenever a property created by this manager changes.

    \sa QtAbstractPropertyManager, QtDoublePropertyManager, QtPointPropertyManager
*/

/*!
    \fn void QtPointFPropertyManager::valueChanged(QtProperty *property, const QPointF &value)

    This signal is emitted whenever a property created by this manager
    changes its value, passing a pointer to the \a property and the
    new \a value as parameters.

    \sa setValue()
*/

/*!
    \fn void QtPointFPropertyManager::decimalsChanged(QtProperty *property, int prec)

    This signal is emitted whenever a property created by this manager
    changes its precision of value, passing a pointer to the
    \a property and the new \a prec value

    \sa setDecimals()
*/

/*!
    Creates a manager with the given \a parent.
*/
QtVector3DPropertyManager::QtVector3DPropertyManager(QObject *parent)
    : QtAbstractPropertyManager(parent) {
    d_ptr = new QtVector3DPropertyManagerPrivate;
    d_ptr->q_ptr = this;

    d_ptr->m_doublePropertyManager = new QtDoublePropertyManager(this);
    connect(d_ptr->m_doublePropertyManager, SIGNAL(valueChanged(QtProperty *, double)),
            this, SLOT(slotDoubleChanged(QtProperty *, double)));
    connect(d_ptr->m_doublePropertyManager, SIGNAL(propertyDestroyed(QtProperty *)),
            this, SLOT(slotPropertyDestroyed(QtProperty *)));
}

/*!
    Destroys this manager, and all the properties it has created.
*/
QtVector3DPropertyManager::~QtVector3DPropertyManager() {
    clear();
    delete d_ptr;
}

/*!
    Returns the manager that creates the nested \e x and \e y
    subproperties.

    In order to provide editing widgets for the subproperties in a
    property browser widget, this manager must be associated with an
    editor factory.

    \sa QtAbstractPropertyBrowser::setFactoryForManager()
*/
QtDoublePropertyManager *QtVector3DPropertyManager::subDoublePropertyManager() const {
    return d_ptr->m_doublePropertyManager;
}

/*!
    Returns the given \a property's value.

    If the given \a property is not managed by this manager, this
    function returns a point with coordinates (0, 0).

    \sa setValue()
*/
QVector3D QtVector3DPropertyManager::value(const QtProperty *property) const {
    return getValue<QVector3D>(d_ptr->m_values, property);
}


QVariant QtVector3DPropertyManager::attributeValue(const QtProperty *property, const QString &attribute) const
{
	if( attribute == "decimals" ) {
		return QVariant(decimals(property));
	} else {
		qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	}
	return QVariant();
}


QVariant QtVector3DPropertyManager::variantValue(const QtProperty *property) const
{
	return QVariant::fromValue(value(property));
}


int QtVector3DPropertyManager::propertyTypeId() const
{
	return QVariant::Vector3D;
}


QList<QtAbstractPropertyManager*> QtVector3DPropertyManager::subPropertyManagers() const
{
	return{ subDoublePropertyManager() };
}


/*!
    Returns the given \a property's precision, in decimals.

    \sa setDecimals()
*/
int QtVector3DPropertyManager::decimals(const QtProperty *property) const {
    return getData<int>(d_ptr->m_values, &QtVector3DPropertyManagerPrivate::Data::decimals, property, 0);
}

/*!
    \reimp
*/
QString QtVector3DPropertyManager::valueText(const QtProperty *property) const {
    const QtVector3DPropertyManagerPrivate::PropertyValueMap::const_iterator it = d_ptr->m_values.constFind(property);
    if (it == d_ptr->m_values.constEnd())
        return QString();
    const QVector3D v = it.value().val;
    const int dec =  it.value().decimals;
    return QString(tr("(%1, %2, %3)").arg(QString::number(v.x(), 'f', dec))
                   .arg(QString::number(v.y(), 'f', dec))
                   .arg(QString::number(v.z(), 'f', dec)));
}

/*!
    \fn void QtPointFPropertyManager::setValue(QtProperty *property, const QPointF &value)

    Sets the value of the given \a property to \a value. Nested
    properties are updated automatically.

    \sa value(), valueChanged()
*/
void QtVector3DPropertyManager::setValue(QtProperty *property, const QVector3D &val) {
    const QtVector3DPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
	if( it == d_ptr->m_values.end() ) {
		return;
	}

	if( it.value().val == val ) {
		return;
	}

    it.value().val = val;
    d_ptr->m_doublePropertyManager->setValue(d_ptr->m_propertyToX[property], val.x());
    d_ptr->m_doublePropertyManager->setValue(d_ptr->m_propertyToY[property], val.y());
    d_ptr->m_doublePropertyManager->setValue(d_ptr->m_propertyToZ[property], val.z());

    emit propertyChanged(property);
    emit valueChanged(property, val);
	emit QtAbstractPropertyManager::valueChanged(property, QVariant(it.value().val));
}


void QtVector3DPropertyManager::setAttribute(QtProperty *property, const QString &attribute, const QVariant &value)
{
	if( attribute == "decimals" ) {
		setDecimals(property, qvariant_cast<int>(value));
	} else {
		qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	}
}


void QtVector3DPropertyManager::setValue(QtProperty *property, const QVariant &val)
{
	Q_ASSERT(val.canConvert<QVector3D>());
	auto s_val = qvariant_cast<QVector3D>(val);
	setValue(property, s_val);
}


/*!
    \fn void QtPointFPropertyManager::setDecimals(QtProperty *property, int prec)

    Sets the precision of the given \a property to \a prec.

    The valid decimal range is 0-13. The default is 2.

    \sa decimals()
*/
void QtVector3DPropertyManager::setDecimals(QtProperty *property, int prec) {
    const QtVector3DPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    QtVector3DPropertyManagerPrivate::Data data = it.value();

    if (prec > 13)
        prec = 13;
    else if (prec < 0)
        prec = 0;

    if (data.decimals == prec)
        return;

    data.decimals = prec;
    d_ptr->m_doublePropertyManager->setDecimals(d_ptr->m_propertyToX[property], prec);
    d_ptr->m_doublePropertyManager->setDecimals(d_ptr->m_propertyToY[property], prec);
    d_ptr->m_doublePropertyManager->setDecimals(d_ptr->m_propertyToZ[property], prec);

    it.value() = data;

    emit decimalsChanged(property, data.decimals);
	emit attributeChanged(property, "decimals", data.decimals);
}

/*!
    \reimp
*/
void QtVector3DPropertyManager::initializeProperty(QtProperty *property) {
    d_ptr->m_values[property] = QtVector3DPropertyManagerPrivate::Data();

    QtProperty *xProp = d_ptr->m_doublePropertyManager->addProperty();
    xProp->setPropertyName(tr("X"));
    d_ptr->m_doublePropertyManager->setDecimals(xProp, decimals(property));
    d_ptr->m_doublePropertyManager->setValue(xProp, 0);
    d_ptr->m_propertyToX[property] = xProp;
    d_ptr->m_xToProperty[xProp] = property;
    property->addSubProperty(xProp);

    QtProperty *yProp = d_ptr->m_doublePropertyManager->addProperty();
    yProp->setPropertyName(tr("Y"));
    d_ptr->m_doublePropertyManager->setDecimals(yProp, decimals(property));
    d_ptr->m_doublePropertyManager->setValue(yProp, 0);
    d_ptr->m_propertyToY[property] = yProp;
    d_ptr->m_yToProperty[yProp] = property;
    property->addSubProperty(yProp);

    QtProperty *zProp = d_ptr->m_doublePropertyManager->addProperty();
    zProp->setPropertyName(tr("Z"));
    d_ptr->m_doublePropertyManager->setDecimals(zProp, decimals(property));
    d_ptr->m_doublePropertyManager->setValue(zProp, 0);
    d_ptr->m_propertyToZ[property] = zProp;
    d_ptr->m_zToProperty[zProp] = property;
    property->addSubProperty(zProp);
}

/*!
    \reimp
*/
void QtVector3DPropertyManager::uninitializeProperty(QtProperty *property) {
    QtProperty *xProp = d_ptr->m_propertyToX[property];
    if (xProp) {
        d_ptr->m_xToProperty.remove(xProp);
        delete xProp;
    }
    d_ptr->m_propertyToX.remove(property);

    QtProperty *yProp = d_ptr->m_propertyToY[property];
    if (yProp) {
        d_ptr->m_yToProperty.remove(yProp);
        delete yProp;
    }
    d_ptr->m_propertyToY.remove(property);

    QtProperty *zProp = d_ptr->m_propertyToZ[property];
    if (zProp) {
        d_ptr->m_zToProperty.remove(zProp);
        delete zProp;
    }
    d_ptr->m_propertyToZ.remove(property);

    d_ptr->m_values.remove(property);
}


/*!
    Returns the manager that creates the nested \e language
    and \e country subproperties.

    In order to provide editing widgets for the mentioned subproperties
    in a property browser widget, this manager must be associated with
    an editor factory.

    \sa QtAbstractPropertyBrowser::setFactoryForManager()
*/
QtEnumPropertyManager *QtLocalePropertyManager::subEnumPropertyManager() const
{
    return d_ptr->m_enumPropertyManager;
}

/*!
    Returns the given \a property's value.

    If the given property is not managed by this manager, this
    function returns the default locale.

    \sa setValue()
*/
QLocale QtLocalePropertyManager::value(const QtProperty *property) const
{
    return d_ptr->m_values.value(property, QLocale());
}


QVariant QtLocalePropertyManager::attributeValue(const QtProperty *property, const QString &attribute) const
{
	Q_UNUSED(property);
	qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	return QVariant();
}


int QtLocalePropertyManager::propertyTypeId() const
{
	return QVariant::Locale;
}


QList<QtAbstractPropertyManager*> QtLocalePropertyManager::subPropertyManagers() const
{
	return{ subEnumPropertyManager() };
}


QVariant QtLocalePropertyManager::variantValue(const QtProperty *property) const
{
	return QVariant::fromValue(value(property));
}


/*!
    \reimp
*/
QString QtLocalePropertyManager::valueText(const QtProperty *property) const
{
    const QtLocalePropertyManagerPrivate::PropertyValueMap::const_iterator it = d_ptr->m_values.constFind(property);
    if (it == d_ptr->m_values.constEnd())
        return QString();

    QLocale loc = it.value();

    int langIdx = 0;
    int countryIdx = 0;
    metaEnumProvider()->localeToIndex(loc.language(), loc.country(), &langIdx, &countryIdx);
    QString str = tr("%1, %2")
            .arg(metaEnumProvider()->languageEnumNames().at(langIdx))
            .arg(metaEnumProvider()->countryEnumNames(loc.language()).at(countryIdx));
    return str;
}

/*!
    \fn void QtLocalePropertyManager::setValue(QtProperty *property, const QLocale &value)

    Sets the value of the given \a property to \a value. Nested
    properties are updated automatically.

    \sa value(), valueChanged()
*/
void QtLocalePropertyManager::setValue(QtProperty *property, const QLocale &val)
{
    const QtLocalePropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    const QLocale loc = it.value();
    if (loc == val)
        return;

    it.value() = val;

    int langIdx = 0;
    int countryIdx = 0;
    metaEnumProvider()->localeToIndex(val.language(), val.country(), &langIdx, &countryIdx);
    if (loc.language() != val.language()) {
        d_ptr->m_enumPropertyManager->setValue(d_ptr->m_propertyToLanguage.value(property), langIdx);
        d_ptr->m_enumPropertyManager->setEnumNames(d_ptr->m_propertyToCountry.value(property),
                    metaEnumProvider()->countryEnumNames(val.language()));
    }
    d_ptr->m_enumPropertyManager->setValue(d_ptr->m_propertyToCountry.value(property), countryIdx);

    emit propertyChanged(property);
    emit valueChanged(property, val);
	emit QtAbstractPropertyManager::valueChanged(property, QVariant(val));
}


void QtLocalePropertyManager::setValue(QtProperty *property, const QVariant &val)
{
	Q_ASSERT(val.canConvert<QLocale>());
	auto s_val = qvariant_cast<QLocale>(val);
	setValue(property, s_val);
}


void QtLocalePropertyManager::setAttribute(QtProperty *property, const QString &attribute, const QVariant &value)
{
	Q_UNUSED(property); Q_UNUSED(value);
	qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
}


/*!
    \reimp
*/
void QtLocalePropertyManager::initializeProperty(QtProperty *property)
{
    QLocale val;
    d_ptr->m_values[property] = val;

    int langIdx = 0;
    int countryIdx = 0;
    metaEnumProvider()->localeToIndex(val.language(), val.country(), &langIdx, &countryIdx);

    QtProperty *languageProp = d_ptr->m_enumPropertyManager->addProperty();
    languageProp->setPropertyName(tr("Language"));
    d_ptr->m_enumPropertyManager->setEnumNames(languageProp, metaEnumProvider()->languageEnumNames());
    d_ptr->m_enumPropertyManager->setValue(languageProp, langIdx);
    d_ptr->m_propertyToLanguage[property] = languageProp;
    d_ptr->m_languageToProperty[languageProp] = property;
    property->addSubProperty(languageProp);

    QtProperty *countryProp = d_ptr->m_enumPropertyManager->addProperty();
    countryProp->setPropertyName(tr("Country"));
    d_ptr->m_enumPropertyManager->setEnumNames(countryProp, metaEnumProvider()->countryEnumNames(val.language()));
    d_ptr->m_enumPropertyManager->setValue(countryProp, countryIdx);
    d_ptr->m_propertyToCountry[property] = countryProp;
    d_ptr->m_countryToProperty[countryProp] = property;
    property->addSubProperty(countryProp);
}

/*!
    \reimp
*/
void QtLocalePropertyManager::uninitializeProperty(QtProperty *property)
{
    QtProperty *languageProp = d_ptr->m_propertyToLanguage[property];
    if (languageProp) {
        d_ptr->m_languageToProperty.remove(languageProp);
        delete languageProp;
    }
    d_ptr->m_propertyToLanguage.remove(property);

    QtProperty *countryProp = d_ptr->m_propertyToCountry[property];
    if (countryProp) {
        d_ptr->m_countryToProperty.remove(countryProp);
        delete countryProp;
    }
    d_ptr->m_propertyToCountry.remove(property);

    d_ptr->m_values.remove(property);
}

// QtPointPropertyManager

void QtPointPropertyManagerPrivate::slotIntChanged(QtProperty *property, int value)
{
    if (QtProperty *xprop = m_xToProperty.value(property, 0)) {
        QPoint p = m_values[xprop];
        p.setX(value);
        q_ptr->setValue(xprop, p);
    } else if (QtProperty *yprop = m_yToProperty.value(property, 0)) {
        QPoint p = m_values[yprop];
        p.setY(value);
        q_ptr->setValue(yprop, p);
    }
}

void QtPointPropertyManagerPrivate::slotPropertyDestroyed(QtProperty *property)
{
    if (QtProperty *pointProp = m_xToProperty.value(property, 0)) {
        m_propertyToX[pointProp] = 0;
        m_xToProperty.remove(property);
    } else if (QtProperty *pointProp = m_yToProperty.value(property, 0)) {
        m_propertyToY[pointProp] = 0;
        m_yToProperty.remove(property);
    }
}

/*! \class QtPointPropertyManager

    \brief The QtPointPropertyManager provides and manages QPoint properties.

    A point property has nested \e x and \e y subproperties. The
    top-level property's value can be retrieved using the value()
    function, and set using the setValue() slot.

    The subproperties are created by a QtIntPropertyManager object. This
    manager can be retrieved using the subIntPropertyManager() function. In
    order to provide editing widgets for the subproperties in a
    property browser widget, this manager must be associated with an
    editor factory.

    In addition, QtPointPropertyManager provides the valueChanged() signal which
    is emitted whenever a property created by this manager changes.

    \sa QtAbstractPropertyManager, QtIntPropertyManager, QtPointFPropertyManager
*/

/*!
    \fn void QtPointPropertyManager::valueChanged(QtProperty *property, const QPoint &value)

    This signal is emitted whenever a property created by this manager
    changes its value, passing a pointer to the \a property and the
    new \a value as parameters.

    \sa setValue()
*/

/*!
    Creates a manager with the given \a parent.
*/
QtPointPropertyManager::QtPointPropertyManager(QObject *parent)
    : QtAbstractPropertyManager(parent)
{
    d_ptr = new QtPointPropertyManagerPrivate;
    d_ptr->q_ptr = this;

    d_ptr->m_intPropertyManager = new QtIntPropertyManager(this);
    connect(d_ptr->m_intPropertyManager, SIGNAL(valueChanged(QtProperty *, int)),
                this, SLOT(slotIntChanged(QtProperty *, int)));
    connect(d_ptr->m_intPropertyManager, SIGNAL(propertyDestroyed(QtProperty *)),
                this, SLOT(slotPropertyDestroyed(QtProperty *)));
}

/*!
    Destroys this manager, and all the properties it has created.
*/
QtPointPropertyManager::~QtPointPropertyManager()
{
    clear();
    delete d_ptr;
}

/*!
    Returns the manager that creates the nested \e x and \e y
    subproperties.

    In order to provide editing widgets for the subproperties in a
    property browser widget, this manager must be associated with an
    editor factory.

    \sa QtAbstractPropertyBrowser::setFactoryForManager()
*/
QtIntPropertyManager *QtPointPropertyManager::subIntPropertyManager() const
{
    return d_ptr->m_intPropertyManager;
}

/*!
    Returns the given \a property's value.

    If the given \a property is not managed by this manager, this
    function returns a point with coordinates (0, 0).

    \sa setValue()
*/
QPoint QtPointPropertyManager::value(const QtProperty *property) const
{
    return d_ptr->m_values.value(property, QPoint());
}


QVariant QtPointPropertyManager::attributeValue(const QtProperty *property, const QString &attribute) const
{
	Q_UNUSED(property);
	qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	return QVariant();
}


int QtPointPropertyManager::propertyTypeId() const
{
	return QVariant::Point;
}


QList<QtAbstractPropertyManager*> QtPointPropertyManager::subPropertyManagers() const
{
	return{ subIntPropertyManager() };
}


QVariant QtPointPropertyManager::variantValue(const QtProperty *property) const
{
	return QVariant::fromValue(value(property));
}


/*!
    \reimp
*/
QString QtPointPropertyManager::valueText(const QtProperty *property) const
{
    const QtPointPropertyManagerPrivate::PropertyValueMap::const_iterator it = d_ptr->m_values.constFind(property);
    if (it == d_ptr->m_values.constEnd())
        return QString();
    const QPoint v = it.value();
    return QString(tr("(%1, %2)").arg(QString::number(v.x()))
                                 .arg(QString::number(v.y())));
}

/*!
    \fn void QtPointPropertyManager::setValue(QtProperty *property, const QPoint &value)

    Sets the value of the given \a property to \a value. Nested
    properties are updated automatically.

    \sa value(), valueChanged()
*/
void QtPointPropertyManager::setValue(QtProperty *property, const QPoint &val)
{
    const QtPointPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    if (it.value() == val)
        return;

    it.value() = val;
    d_ptr->m_intPropertyManager->setValue(d_ptr->m_propertyToX[property], val.x());
    d_ptr->m_intPropertyManager->setValue(d_ptr->m_propertyToY[property], val.y());

    emit propertyChanged(property);
    emit valueChanged(property, val);
	emit QtAbstractPropertyManager::valueChanged(property, QVariant(val));
}


void QtPointPropertyManager::setValue(QtProperty *property, const QVariant &val)
{
	Q_ASSERT(val.canConvert<QPoint>());
	auto s_val = qvariant_cast<QPoint>(val);
	setValue(property, s_val);
}


void QtPointPropertyManager::setAttribute(QtProperty *property, const QString &attribute, const QVariant &value)
{
	Q_UNUSED(property); Q_UNUSED(value);
	qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
}


/*!
    \reimp
*/
void QtPointPropertyManager::initializeProperty(QtProperty *property)
{
    d_ptr->m_values[property] = QPoint(0, 0);

    QtProperty *xProp = d_ptr->m_intPropertyManager->addProperty();
    xProp->setPropertyName(tr("X"));
    d_ptr->m_intPropertyManager->setValue(xProp, 0);
    d_ptr->m_propertyToX[property] = xProp;
    d_ptr->m_xToProperty[xProp] = property;
    property->addSubProperty(xProp);

    QtProperty *yProp = d_ptr->m_intPropertyManager->addProperty();
    yProp->setPropertyName(tr("Y"));
    d_ptr->m_intPropertyManager->setValue(yProp, 0);
    d_ptr->m_propertyToY[property] = yProp;
    d_ptr->m_yToProperty[yProp] = property;
    property->addSubProperty(yProp);
}

/*!
    \reimp
*/
void QtPointPropertyManager::uninitializeProperty(QtProperty *property)
{
    QtProperty *xProp = d_ptr->m_propertyToX[property];
    if (xProp) {
        d_ptr->m_xToProperty.remove(xProp);
        delete xProp;
    }
    d_ptr->m_propertyToX.remove(property);

    QtProperty *yProp = d_ptr->m_propertyToY[property];
    if (yProp) {
        d_ptr->m_yToProperty.remove(yProp);
        delete yProp;
    }
    d_ptr->m_propertyToY.remove(property);

    d_ptr->m_values.remove(property);
}

// QtPointFPropertyManager

void QtPointFPropertyManagerPrivate::slotDoubleChanged(QtProperty *property, double value)
{
    if (QtProperty *prop = m_xToProperty.value(property, 0)) {
        QPointF p = m_values[prop].val;
        p.setX(value);
        q_ptr->setValue(prop, p);
    } else if (QtProperty *prop = m_yToProperty.value(property, 0)) {
        QPointF p = m_values[prop].val;
        p.setY(value);
        q_ptr->setValue(prop, p);
    }
}

void QtPointFPropertyManagerPrivate::slotPropertyDestroyed(QtProperty *property)
{
    if (QtProperty *pointProp  = m_xToProperty.value(property, 0)) {
        m_propertyToX[pointProp] = 0;
        m_xToProperty.remove(property);
    } else if (QtProperty *pointProp = m_yToProperty.value(property, 0)) {
        m_propertyToY[pointProp] = 0;
        m_yToProperty.remove(property);
    }
}

/*! \class QtPointFPropertyManager

    \brief The QtPointFPropertyManager provides and manages QPointF properties.

    A point property has nested \e x and \e y subproperties. The
    top-level property's value can be retrieved using the value()
    function, and set using the setValue() slot.

    The subproperties are created by a QtDoublePropertyManager object. This
    manager can be retrieved using the subDoublePropertyManager() function. In
    order to provide editing widgets for the subproperties in a
    property browser widget, this manager must be associated with an
    editor factory.

    In addition, QtPointFPropertyManager provides the valueChanged() signal which
    is emitted whenever a property created by this manager changes.

    \sa QtAbstractPropertyManager, QtDoublePropertyManager, QtPointPropertyManager
*/

/*!
    \fn void QtPointFPropertyManager::valueChanged(QtProperty *property, const QPointF &value)

    This signal is emitted whenever a property created by this manager
    changes its value, passing a pointer to the \a property and the
    new \a value as parameters.

    \sa setValue()
*/

/*!
    \fn void QtPointFPropertyManager::decimalsChanged(QtProperty *property, int prec)

    This signal is emitted whenever a property created by this manager
    changes its precision of value, passing a pointer to the
    \a property and the new \a prec value

    \sa setDecimals()
*/

/*!
    Creates a manager with the given \a parent.
*/
QtPointFPropertyManager::QtPointFPropertyManager(QObject *parent)
    : QtAbstractPropertyManager(parent)
{
    d_ptr = new QtPointFPropertyManagerPrivate;
    d_ptr->q_ptr = this;

    d_ptr->m_doublePropertyManager = new QtDoublePropertyManager(this);
    connect(d_ptr->m_doublePropertyManager, SIGNAL(valueChanged(QtProperty *, double)),
                this, SLOT(slotDoubleChanged(QtProperty *, double)));
    connect(d_ptr->m_doublePropertyManager, SIGNAL(propertyDestroyed(QtProperty *)),
                this, SLOT(slotPropertyDestroyed(QtProperty *)));
}

/*!
    Destroys this manager, and all the properties it has created.
*/
QtPointFPropertyManager::~QtPointFPropertyManager()
{
    clear();
    delete d_ptr;
}

/*!
    Returns the manager that creates the nested \e x and \e y
    subproperties.

    In order to provide editing widgets for the subproperties in a
    property browser widget, this manager must be associated with an
    editor factory.

    \sa QtAbstractPropertyBrowser::setFactoryForManager()
*/
QtDoublePropertyManager *QtPointFPropertyManager::subDoublePropertyManager() const
{
    return d_ptr->m_doublePropertyManager;
}

/*!
    Returns the given \a property's value.

    If the given \a property is not managed by this manager, this
    function returns a point with coordinates (0, 0).

    \sa setValue()
*/
QPointF QtPointFPropertyManager::value(const QtProperty *property) const
{
    return getValue<QPointF>(d_ptr->m_values, property);
}


QVariant QtPointFPropertyManager::variantValue(const QtProperty *property) const
{
	return QVariant::fromValue(value(property));
}


QVariant QtPointFPropertyManager::attributeValue(const QtProperty *property, const QString &attribute) const
{
	if( attribute == "decimals" ) {
		return QVariant(decimals(property));
	} else {
		qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	}
	return QVariant();
}


int QtPointFPropertyManager::propertyTypeId() const
{
	return QVariant::PointF;
}


QList<QtAbstractPropertyManager*> QtPointFPropertyManager::subPropertyManagers() const
{
	return{ subDoublePropertyManager() };
}


/*!
    Returns the given \a property's precision, in decimals.

    \sa setDecimals()
*/
int QtPointFPropertyManager::decimals(const QtProperty *property) const
{
    return getData<int>(d_ptr->m_values, &QtPointFPropertyManagerPrivate::Data::decimals, property, 0);
}

/*!
    \reimp
*/
QString QtPointFPropertyManager::valueText(const QtProperty *property) const
{
    const QtPointFPropertyManagerPrivate::PropertyValueMap::const_iterator it = d_ptr->m_values.constFind(property);
    if (it == d_ptr->m_values.constEnd())
        return QString();
    const QPointF v = it.value().val;
    const int dec =  it.value().decimals;
    return QString(tr("(%1, %2)").arg(QString::number(v.x(), 'f', dec))
                                 .arg(QString::number(v.y(), 'f', dec)));
}

/*!
    \fn void QtPointFPropertyManager::setValue(QtProperty *property, const QPointF &value)

    Sets the value of the given \a property to \a value. Nested
    properties are updated automatically.

    \sa value(), valueChanged()
*/
void QtPointFPropertyManager::setValue(QtProperty *property, const QPointF &val)
{
    const QtPointFPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    if (it.value().val == val)
        return;

    it.value().val = val;
    d_ptr->m_doublePropertyManager->setValue(d_ptr->m_propertyToX[property], val.x());
    d_ptr->m_doublePropertyManager->setValue(d_ptr->m_propertyToY[property], val.y());

    emit propertyChanged(property);
    emit valueChanged(property, val);
	emit QtAbstractPropertyManager::valueChanged(property, QVariant(val));
}


void QtPointFPropertyManager::setValue(QtProperty *property, const QVariant &val)
{
	Q_ASSERT(val.canConvert<QPointF>());
	auto s_val = qvariant_cast<QPointF>(val);
	setValue(property, s_val);
}


void QtPointFPropertyManager::setAttribute(QtProperty *property, const QString &attribute, const QVariant &value)
{
	if( attribute == "decimals" ) {
		setDecimals(property, qvariant_cast<int>(value));
	} else {
		qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	}
}


/*!
    \fn void QtPointFPropertyManager::setDecimals(QtProperty *property, int prec)

    Sets the precision of the given \a property to \a prec.

    The valid decimal range is 0-13. The default is 2.

    \sa decimals()
*/
void QtPointFPropertyManager::setDecimals(QtProperty *property, int prec)
{
    const QtPointFPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    QtPointFPropertyManagerPrivate::Data data = it.value();

    if (prec > 13)
        prec = 13;
    else if (prec < 0)
        prec = 0;

    if (data.decimals == prec)
        return;

    data.decimals = prec;
    d_ptr->m_doublePropertyManager->setDecimals(d_ptr->m_propertyToX[property], prec);
    d_ptr->m_doublePropertyManager->setDecimals(d_ptr->m_propertyToY[property], prec);

    it.value() = data;

    emit decimalsChanged(property, data.decimals);
	emit attributeChanged(property, "decimals", data.decimals);
}

/*!
    \reimp
*/
void QtPointFPropertyManager::initializeProperty(QtProperty *property)
{
    d_ptr->m_values[property] = QtPointFPropertyManagerPrivate::Data();

    QtProperty *xProp = d_ptr->m_doublePropertyManager->addProperty();
    xProp->setPropertyName(tr("X"));
    d_ptr->m_doublePropertyManager->setDecimals(xProp, decimals(property));
    d_ptr->m_doublePropertyManager->setValue(xProp, 0);
    d_ptr->m_propertyToX[property] = xProp;
    d_ptr->m_xToProperty[xProp] = property;
    property->addSubProperty(xProp);

    QtProperty *yProp = d_ptr->m_doublePropertyManager->addProperty();
    yProp->setPropertyName(tr("Y"));
    d_ptr->m_doublePropertyManager->setDecimals(yProp, decimals(property));
    d_ptr->m_doublePropertyManager->setValue(yProp, 0);
    d_ptr->m_propertyToY[property] = yProp;
    d_ptr->m_yToProperty[yProp] = property;
    property->addSubProperty(yProp);
}

/*!
    \reimp
*/
void QtPointFPropertyManager::uninitializeProperty(QtProperty *property)
{
    QtProperty *xProp = d_ptr->m_propertyToX[property];
    if (xProp) {
        d_ptr->m_xToProperty.remove(xProp);
        delete xProp;
    }
    d_ptr->m_propertyToX.remove(property);

    QtProperty *yProp = d_ptr->m_propertyToY[property];
    if (yProp) {
        d_ptr->m_yToProperty.remove(yProp);
        delete yProp;
    }
    d_ptr->m_propertyToY.remove(property);

    d_ptr->m_values.remove(property);
}

// QtSizePropertyManager

void QtSizePropertyManagerPrivate::slotIntChanged(QtProperty *property, int value)
{
    if (QtProperty *prop = m_wToProperty.value(property, 0)) {
        QSize s = m_values[prop].val;
        s.setWidth(value);
        q_ptr->setValue(prop, s);
    } else if (QtProperty *prop = m_hToProperty.value(property, 0)) {
        QSize s = m_values[prop].val;
        s.setHeight(value);
        q_ptr->setValue(prop, s);
    }
}

void QtSizePropertyManagerPrivate::slotPropertyDestroyed(QtProperty *property)
{
    if (QtProperty *pointProp = m_wToProperty.value(property, 0)) {
        m_propertyToW[pointProp] = 0;
        m_wToProperty.remove(property);
    } else if (QtProperty *pointProp = m_hToProperty.value(property, 0)) {
        m_propertyToH[pointProp] = 0;
        m_hToProperty.remove(property);
    }
}

void QtSizePropertyManagerPrivate::setValue(QtProperty *property, const QSize &val)
{
    m_intPropertyManager->setValue(m_propertyToW.value(property), val.width());
    m_intPropertyManager->setValue(m_propertyToH.value(property), val.height());
}

void QtSizePropertyManagerPrivate::setRange(QtProperty *property,
                const QSize &minVal, const QSize &maxVal, const QSize &val)
{
    QtProperty *wProperty = m_propertyToW.value(property);
    QtProperty *hProperty = m_propertyToH.value(property);
    m_intPropertyManager->setRange(wProperty, minVal.width(), maxVal.width());
    m_intPropertyManager->setValue(wProperty, val.width());
    m_intPropertyManager->setRange(hProperty, minVal.height(), maxVal.height());
    m_intPropertyManager->setValue(hProperty, val.height());
}

/*!
    \class QtSizePropertyManager

    \brief The QtSizePropertyManager provides and manages QSize properties.

    A size property has nested \e width and \e height
    subproperties. The top-level property's value can be retrieved
    using the value() function, and set using the setValue() slot.

    The subproperties are created by a QtIntPropertyManager object. This
    manager can be retrieved using the subIntPropertyManager() function. In
    order to provide editing widgets for the subproperties in a
    property browser widget, this manager must be associated with an
    editor factory.

    A size property also has a range of valid values defined by a
    minimum size and a maximum size. These sizes can be retrieved
    using the minimum() and the maximum() functions, and set using the
    setMinimum() and setMaximum() slots. Alternatively, the range can
    be defined in one go using the setRange() slot.

    In addition, QtSizePropertyManager provides the valueChanged() signal
    which is emitted whenever a property created by this manager
    changes, and the rangeChanged() signal which is emitted whenever
    such a property changes its range of valid sizes.

    \sa QtAbstractPropertyManager, QtIntPropertyManager, QtSizeFPropertyManager
*/

/*!
    \fn void QtSizePropertyManager::valueChanged(QtProperty *property, const QSize &value)

    This signal is emitted whenever a property created by this manager
    changes its value, passing a pointer to the \a property and the new
    \a value as parameters.

    \sa setValue()
*/

/*!
    \fn void QtSizePropertyManager::rangeChanged(QtProperty *property, const QSize &minimum, const QSize &maximum)

    This signal is emitted whenever a property created by this manager
    changes its range of valid sizes, passing a pointer to the \a
    property and the new \a minimum and \a maximum sizes.

    \sa setRange()
*/

/*!
    Creates a manager with the given \a parent.
*/
QtSizePropertyManager::QtSizePropertyManager(QObject *parent)
    : QtAbstractPropertyManager(parent)
{
    d_ptr = new QtSizePropertyManagerPrivate;
    d_ptr->q_ptr = this;

    d_ptr->m_intPropertyManager = new QtIntPropertyManager(this);
    connect(d_ptr->m_intPropertyManager, SIGNAL(valueChanged(QtProperty *, int)),
                this, SLOT(slotIntChanged(QtProperty *, int)));
    connect(d_ptr->m_intPropertyManager, SIGNAL(propertyDestroyed(QtProperty *)),
                this, SLOT(slotPropertyDestroyed(QtProperty *)));
}

/*!
    Destroys this manager, and all the properties it has created.
*/
QtSizePropertyManager::~QtSizePropertyManager()
{
    clear();
    delete d_ptr;
}

/*!
    Returns the manager that creates the nested \e width and \e height
    subproperties.

    In order to provide editing widgets for the \e width and \e height
    properties in a property browser widget, this manager must be
    associated with an editor factory.

    \sa QtAbstractPropertyBrowser::setFactoryForManager()
*/
QtIntPropertyManager *QtSizePropertyManager::subIntPropertyManager() const
{
    return d_ptr->m_intPropertyManager;
}

/*!
    Returns the given \a property's value.

    If the given \a property is not managed by this manager, this
    function returns an invalid size

    \sa setValue()
*/
QSize QtSizePropertyManager::value(const QtProperty *property) const
{
    return getValue<QSize>(d_ptr->m_values, property);
}


QVariant QtSizePropertyManager::variantValue(const QtProperty *property) const
{
	return QVariant::fromValue(value(property));
}


QVariant QtSizePropertyManager::attributeValue(const QtProperty *property, const QString &attribute) const
{
	if( attribute == "minimum" ) {
		return QVariant(minimum(property));
	} else if( attribute == "maximum" ) {
		return QVariant(maximum(property));
	} else {
		qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	}
	return QVariant();
}


int QtSizePropertyManager::propertyTypeId() const
{
	return QVariant::Size;
}


QList<QtAbstractPropertyManager*> QtSizePropertyManager::subPropertyManagers() const
{
	return{ subIntPropertyManager() };
}


/*!
    Returns the given \a property's minimum size value.

    \sa setMinimum(), maximum(), setRange()
*/
QSize QtSizePropertyManager::minimum(const QtProperty *property) const
{
    return getMinimum<QSize>(d_ptr->m_values, property);
}

/*!
    Returns the given \a property's maximum size value.

    \sa setMaximum(), minimum(), setRange()
*/
QSize QtSizePropertyManager::maximum(const QtProperty *property) const
{
    return getMaximum<QSize>(d_ptr->m_values, property);
}

/*!
    \reimp
*/
QString QtSizePropertyManager::valueText(const QtProperty *property) const
{
    const QtSizePropertyManagerPrivate::PropertyValueMap::const_iterator it = d_ptr->m_values.constFind(property);
    if (it == d_ptr->m_values.constEnd())
        return QString();
    const QSize v = it.value().val;
    return QString(tr("%1 x %2").arg(QString::number(v.width()))
                                .arg(QString::number(v.height())));
}

/*!
    \fn void QtSizePropertyManager::setValue(QtProperty *property, const QSize &value)

    Sets the value of the given \a property to \a value.

    If the specified \a value is not valid according to the given \a
    property's size range, the \a value is adjusted to the nearest
    valid value within the size range.

    \sa value(), setRange(), valueChanged()
*/
void QtSizePropertyManager::setValue(QtProperty *property, const QSize &val)
{
    setValueInRange<const QSize &, QtSizePropertyManagerPrivate, QtSizePropertyManager, const QSize>(this, d_ptr,
                &QtSizePropertyManager::propertyChanged,
                &QtSizePropertyManager::valueChanged,
                &QtAbstractPropertyManager::valueChanged,
                property, val, &QtSizePropertyManagerPrivate::setValue);
}


void QtSizePropertyManager::setValue(QtProperty *property, const QVariant &val)
{
	Q_ASSERT(val.canConvert<QSize>());
	auto s_val = qvariant_cast<QSize>(val);
	setValue(property, s_val);
}


void QtSizePropertyManager::setAttribute(QtProperty *property, const QString &attribute, const QVariant &value)
{
	if( attribute == "minimum" ) {
		setMinimum(property, qvariant_cast<QSize>(value));
	} else if( attribute == "maximum" ) {
		setMaximum(property, qvariant_cast<QSize>(value));
	} else {
		qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	}
}


/*!
    Sets the minimum size value for the given \a property to \a minVal.

    When setting the minimum size value, the maximum and current
    values are adjusted if necessary (ensuring that the size range
    remains valid and that the current value is within the range).

    \sa minimum(), setRange(), rangeChanged()
*/
void QtSizePropertyManager::setMinimum(QtProperty *property, const QSize &minVal)
{
	auto it = d_ptr->m_values.find(property);
	if( it == d_ptr->m_values.end() ) {
		return;
	}

	auto& data = it.value();

	if( data.minimumValue() == minVal ) {
		return;
	}

	const auto oldMaxVal = data.maxVal;
	const auto oldVal = data.val;

	data.setMinimumValue(minVal);

	emit rangeChanged(property, data.minVal, data.maxVal);
	emit attributeChanged(property, "minimum", data.minVal);

	if( oldMaxVal != data.maxVal ) {
		emit attributeChanged(property, "maximum", data.maxVal);
	}

	if( data.val != oldVal ) {
		emit propertyChanged(property);
		emit valueChanged(property, data.val);
		emit QtAbstractPropertyManager::valueChanged(property, data.val);
	}
}

/*!
    Sets the maximum size value for the given \a property to \a maxVal.

    When setting the maximum size value, the minimum and current
    values are adjusted if necessary (ensuring that the size range
    remains valid and that the current value is within the range).

    \sa maximum(), setRange(), rangeChanged()
*/
void QtSizePropertyManager::setMaximum(QtProperty *property, const QSize &maxVal)
{
	auto it = d_ptr->m_values.find(property);
	if( it == d_ptr->m_values.end() ) {
		return;
	}

	auto& data = it.value();

	if( data.maximumValue() == maxVal ) {
		return;
	}

	const auto oldMinVal = data.minVal;
	const auto oldVal = data.val;

	data.setMaximumValue(maxVal);

	emit rangeChanged(property, data.minVal, data.maxVal);
	emit attributeChanged(property, "maximum", data.maxVal);

	if( oldMinVal != data.minVal ) {
		emit attributeChanged(property, "minimum", data.minVal);
	}

	if( data.val != oldVal ) {
		emit propertyChanged(property);
		emit valueChanged(property, data.val);
		emit QtAbstractPropertyManager::valueChanged(property, data.val);
	}
}

/*!
    \fn void QtSizePropertyManager::setRange(QtProperty *property, const QSize &minimum, const QSize &maximum)

    Sets the range of valid values.

    This is a convenience function defining the range of valid values
    in one go; setting the \a minimum and \a maximum values for the
    given \a property with a single function call.

    When setting a new range, the current value is adjusted if
    necessary (ensuring that the value remains within the range).

    \sa setMinimum(), setMaximum(), rangeChanged()
*/
void QtSizePropertyManager::setRange(QtProperty *property, const QSize &_minVal, const QSize &_maxVal)
{
	auto it = d_ptr->m_values.find(property);
	if( it == d_ptr->m_values.end() ) {
		return;
	}

	auto minVal = _minVal;
	auto maxVal = _maxVal;

	orderBorders(minVal, maxVal);

	auto& data = it.value();

	if( data.minimumValue() == minVal && data.maximumValue() == maxVal ) {
		return;
	}

	const auto oldMinVal = data.minVal;
	const auto oldMaxVal = data.maxVal;
	const auto oldVal = data.val;

	data.setMinimumValue(minVal);
	data.setMaximumValue(maxVal);

	emit rangeChanged(property, data.minVal, data.maxVal);

	if( oldMinVal != data.minVal ) {
		emit attributeChanged(property, "minimum", data.minVal);
	}
	if( oldMaxVal != data.maxVal ) {
		emit attributeChanged(property, "maximum", data.maxVal);
	}
	if( data.val != oldVal ) {
		emit propertyChanged(property);
		emit valueChanged(property, data.val);
		emit QtAbstractPropertyManager::valueChanged(property, data.val);
	}
}

/*!
    \reimp
*/
void QtSizePropertyManager::initializeProperty(QtProperty *property)
{
    d_ptr->m_values[property] = QtSizePropertyManagerPrivate::Data();

    QtProperty *wProp = d_ptr->m_intPropertyManager->addProperty();
    wProp->setPropertyName(tr("Width"));
    d_ptr->m_intPropertyManager->setValue(wProp, 0);
    d_ptr->m_intPropertyManager->setMinimum(wProp, 0);
    d_ptr->m_propertyToW[property] = wProp;
    d_ptr->m_wToProperty[wProp] = property;
    property->addSubProperty(wProp);

    QtProperty *hProp = d_ptr->m_intPropertyManager->addProperty();
    hProp->setPropertyName(tr("Height"));
    d_ptr->m_intPropertyManager->setValue(hProp, 0);
    d_ptr->m_intPropertyManager->setMinimum(hProp, 0);
    d_ptr->m_propertyToH[property] = hProp;
    d_ptr->m_hToProperty[hProp] = property;
    property->addSubProperty(hProp);
}

/*!
    \reimp
*/
void QtSizePropertyManager::uninitializeProperty(QtProperty *property)
{
    QtProperty *wProp = d_ptr->m_propertyToW[property];
    if (wProp) {
        d_ptr->m_wToProperty.remove(wProp);
        delete wProp;
    }
    d_ptr->m_propertyToW.remove(property);

    QtProperty *hProp = d_ptr->m_propertyToH[property];
    if (hProp) {
        d_ptr->m_hToProperty.remove(hProp);
        delete hProp;
    }
    d_ptr->m_propertyToH.remove(property);

    d_ptr->m_values.remove(property);
}

// QtSizeFPropertyManager

void QtSizeFPropertyManagerPrivate::slotDoubleChanged(QtProperty *property, double value)
{
    if (QtProperty *prop = m_wToProperty.value(property, 0)) {
        QSizeF s = m_values[prop].val;
        s.setWidth(value);
        q_ptr->setValue(prop, s);
    } else if (QtProperty *prop = m_hToProperty.value(property, 0)) {
        QSizeF s = m_values[prop].val;
        s.setHeight(value);
        q_ptr->setValue(prop, s);
    }
}

void QtSizeFPropertyManagerPrivate::slotPropertyDestroyed(QtProperty *property)
{
    if (QtProperty *pointProp = m_wToProperty.value(property, 0)) {
        m_propertyToW[pointProp] = 0;
        m_wToProperty.remove(property);
    } else if (QtProperty *pointProp = m_hToProperty.value(property, 0)) {
        m_propertyToH[pointProp] = 0;
        m_hToProperty.remove(property);
    }
}

void QtSizeFPropertyManagerPrivate::setValue(QtProperty *property, const QSizeF &val)
{
    m_doublePropertyManager->setValue(m_propertyToW.value(property), val.width());
    m_doublePropertyManager->setValue(m_propertyToH.value(property), val.height());
}

void QtSizeFPropertyManagerPrivate::setRange(QtProperty *property,
                const QSizeF &minVal, const QSizeF &maxVal, const QSizeF &val)
{
    m_doublePropertyManager->setRange(m_propertyToW[property], minVal.width(), maxVal.width());
    m_doublePropertyManager->setValue(m_propertyToW[property], val.width());
    m_doublePropertyManager->setRange(m_propertyToH[property], minVal.height(), maxVal.height());
    m_doublePropertyManager->setValue(m_propertyToH[property], val.height());
}

/*!
    \class QtSizeFPropertyManager

    \brief The QtSizeFPropertyManager provides and manages QSizeF properties.

    A size property has nested \e width and \e height
    subproperties. The top-level property's value can be retrieved
    using the value() function, and set using the setValue() slot.

    The subproperties are created by a QtDoublePropertyManager object. This
    manager can be retrieved using the subDoublePropertyManager() function. In
    order to provide editing widgets for the subproperties in a
    property browser widget, this manager must be associated with an
    editor factory.

    A size property also has a range of valid values defined by a
    minimum size and a maximum size. These sizes can be retrieved
    using the minimum() and the maximum() functions, and set using the
    setMinimum() and setMaximum() slots. Alternatively, the range can
    be defined in one go using the setRange() slot.

    In addition, QtSizeFPropertyManager provides the valueChanged() signal
    which is emitted whenever a property created by this manager
    changes, and the rangeChanged() signal which is emitted whenever
    such a property changes its range of valid sizes.

    \sa QtAbstractPropertyManager, QtDoublePropertyManager, QtSizePropertyManager
*/

/*!
    \fn void QtSizeFPropertyManager::valueChanged(QtProperty *property, const QSizeF &value)

    This signal is emitted whenever a property created by this manager
    changes its value, passing a pointer to the \a property and the new
    \a value as parameters.

    \sa setValue()
*/

/*!
    \fn void QtSizeFPropertyManager::rangeChanged(QtProperty *property, const QSizeF &minimum, const QSizeF &maximum)

    This signal is emitted whenever a property created by this manager
    changes its range of valid sizes, passing a pointer to the \a
    property and the new \a minimum and \a maximum sizes.

    \sa setRange()
*/

/*!
    \fn void QtSizeFPropertyManager::decimalsChanged(QtProperty *property, int prec)

    This signal is emitted whenever a property created by this manager
    changes its precision of value, passing a pointer to the
    \a property and the new \a prec value

    \sa setDecimals()
*/

/*!
    Creates a manager with the given \a parent.
*/
QtSizeFPropertyManager::QtSizeFPropertyManager(QObject *parent)
    : QtAbstractPropertyManager(parent)
{
    d_ptr = new QtSizeFPropertyManagerPrivate;
    d_ptr->q_ptr = this;

    d_ptr->m_doublePropertyManager = new QtDoublePropertyManager(this);
    connect(d_ptr->m_doublePropertyManager, SIGNAL(valueChanged(QtProperty *, double)),
                this, SLOT(slotDoubleChanged(QtProperty *, double)));
    connect(d_ptr->m_doublePropertyManager, SIGNAL(propertyDestroyed(QtProperty *)),
                this, SLOT(slotPropertyDestroyed(QtProperty *)));
}

/*!
    Destroys this manager, and all the properties it has created.
*/
QtSizeFPropertyManager::~QtSizeFPropertyManager()
{
    clear();
    delete d_ptr;
}

/*!
    Returns the manager that creates the nested \e width and \e height
    subproperties.

    In order to provide editing widgets for the \e width and \e height
    properties in a property browser widget, this manager must be
    associated with an editor factory.

    \sa QtAbstractPropertyBrowser::setFactoryForManager()
*/
QtDoublePropertyManager *QtSizeFPropertyManager::subDoublePropertyManager() const
{
    return d_ptr->m_doublePropertyManager;
}

/*!
    Returns the given \a property's value.

    If the given \a property is not managed by this manager, this
    function returns an invalid size

    \sa setValue()
*/
QSizeF QtSizeFPropertyManager::value(const QtProperty *property) const
{
    return getValue<QSizeF>(d_ptr->m_values, property);
}


QVariant QtSizeFPropertyManager::variantValue(const QtProperty *property) const
{
	return QVariant::fromValue(value(property));
}


/*!
    Returns the given \a property's precision, in decimals.

    \sa setDecimals()
*/
int QtSizeFPropertyManager::decimals(const QtProperty *property) const
{
    return getData<int>(d_ptr->m_values, &QtSizeFPropertyManagerPrivate::Data::decimals, property, 0);
}


QVariant QtSizeFPropertyManager::attributeValue(const QtProperty *property, const QString &attribute) const
{
	if( attribute == "minimum" ) {
		return QVariant(minimum(property));
	} else if( attribute == "maximum" ) {
		return QVariant(maximum(property));
	} else if( attribute == "decimals" ) {
		return QVariant(decimals(property));
	} else {
		qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	}
	return QVariant();
}


int QtSizeFPropertyManager::propertyTypeId() const
{
	return QVariant::SizeF;
}


QList<QtAbstractPropertyManager*> QtSizeFPropertyManager::subPropertyManagers() const
{
	return{ subDoublePropertyManager() };
}


/*!
    Returns the given \a property's minimum size value.

    \sa setMinimum(), maximum(), setRange()
*/
QSizeF QtSizeFPropertyManager::minimum(const QtProperty *property) const
{
    return getMinimum<QSizeF>(d_ptr->m_values, property);
}

/*!
    Returns the given \a property's maximum size value.

    \sa setMaximum(), minimum(), setRange()
*/
QSizeF QtSizeFPropertyManager::maximum(const QtProperty *property) const
{
    return getMaximum<QSizeF>(d_ptr->m_values, property);
}

/*!
    \reimp
*/
QString QtSizeFPropertyManager::valueText(const QtProperty *property) const
{
    const QtSizeFPropertyManagerPrivate::PropertyValueMap::const_iterator it = d_ptr->m_values.constFind(property);
    if (it == d_ptr->m_values.constEnd())
        return QString();
    const QSizeF v = it.value().val;
    const int dec = it.value().decimals;
    return QString(tr("%1 x %2").arg(QString::number(v.width(), 'f', dec))
                                .arg(QString::number(v.height(), 'f', dec)));
}

/*!
    \fn void QtSizeFPropertyManager::setValue(QtProperty *property, const QSizeF &value)

    Sets the value of the given \a property to \a value.

    If the specified \a value is not valid according to the given \a
    property's size range, the \a value is adjusted to the nearest
    valid value within the size range.

    \sa value(), setRange(), valueChanged()
*/
void QtSizeFPropertyManager::setValue(QtProperty *property, const QSizeF &val)
{
    setValueInRange<const QSizeF &, QtSizeFPropertyManagerPrivate, QtSizeFPropertyManager, QSizeF>(this, d_ptr,
                &QtSizeFPropertyManager::propertyChanged,
                &QtSizeFPropertyManager::valueChanged,
                &QtAbstractPropertyManager::valueChanged,
                property, val, &QtSizeFPropertyManagerPrivate::setValue);
}


void QtSizeFPropertyManager::setValue(QtProperty *property, const QVariant &val)
{
	Q_ASSERT(val.canConvert<QSizeF>());
	auto s_val = qvariant_cast<QSizeF>(val);
	setValue(property, s_val);
}


void QtSizeFPropertyManager::setAttribute(QtProperty *property, const QString &attribute, const QVariant &value)
{
	if( attribute == "minimum" ) {
		setMinimum(property, qvariant_cast<QSizeF>(value));
	} else if( attribute == "maximum" ) {
		setMaximum(property, qvariant_cast<QSizeF>(value));
	} else if( attribute == "decimals" ) {
		setDecimals(property, qvariant_cast<int>(value));
	} else {
		qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	}
}


/*!
    \fn void QtSizeFPropertyManager::setDecimals(QtProperty *property, int prec)

    Sets the precision of the given \a property to \a prec.

    The valid decimal range is 0-13. The default is 2.

    \sa decimals()
*/
void QtSizeFPropertyManager::setDecimals(QtProperty *property, int prec)
{
    const QtSizeFPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    QtSizeFPropertyManagerPrivate::Data data = it.value();

    if (prec > 13)
        prec = 13;
    else if (prec < 0)
        prec = 0;

    if (data.decimals == prec)
        return;

    data.decimals = prec;
    d_ptr->m_doublePropertyManager->setDecimals(d_ptr->m_propertyToW[property], prec);
    d_ptr->m_doublePropertyManager->setDecimals(d_ptr->m_propertyToH[property], prec);

    it.value() = data;

    emit decimalsChanged(property, data.decimals);
	emit attributeChanged(property, "decimals", data.decimals);
}

/*!
    Sets the minimum size value for the given \a property to \a minVal.

    When setting the minimum size value, the maximum and current
    values are adjusted if necessary (ensuring that the size range
    remains valid and that the current value is within the range).

    \sa minimum(), setRange(), rangeChanged()
*/
void QtSizeFPropertyManager::setMinimum(QtProperty *property, const QSizeF &minVal)
{
	auto it = d_ptr->m_values.find(property);
	if( it == d_ptr->m_values.end() ) {
		return;
	}

	auto& data = it.value();

	if( data.minimumValue() == minVal ) {
		return;
	}

	const auto oldMaxVal = data.maxVal;
	const auto oldVal = data.val;

	data.setMinimumValue(minVal);

	emit rangeChanged(property, data.minVal, data.maxVal);
	emit attributeChanged(property, "minimum", data.minVal);

	if( oldMaxVal != data.maxVal ) {
		emit attributeChanged(property, "maximum", data.maxVal);
	}

	if( data.val != oldVal ) {
		emit propertyChanged(property);
		emit valueChanged(property, data.val);
		emit QtAbstractPropertyManager::valueChanged(property, data.val);
	}
}

/*!
    Sets the maximum size value for the given \a property to \a maxVal.

    When setting the maximum size value, the minimum and current
    values are adjusted if necessary (ensuring that the size range
    remains valid and that the current value is within the range).

    \sa maximum(), setRange(), rangeChanged()
*/
void QtSizeFPropertyManager::setMaximum(QtProperty *property, const QSizeF &maxVal)
{
	auto it = d_ptr->m_values.find(property);
	if( it == d_ptr->m_values.end() ) {
		return;
	}

	auto& data = it.value();

	if( data.maximumValue() == maxVal ) {
		return;
	}

	const auto oldMinVal = data.minVal;
	const auto oldVal = data.val;

	data.setMaximumValue(maxVal);

	emit rangeChanged(property, data.minVal, data.maxVal);
	emit attributeChanged(property, "maximum", data.maxVal);

	if( oldMinVal != data.minVal ) {
		emit attributeChanged(property, "minimum", data.minVal);
	}

	if( data.val != oldVal ) {
		emit propertyChanged(property);
		emit valueChanged(property, data.val);
		emit QtAbstractPropertyManager::valueChanged(property, data.val);
	}
}

/*!
    \fn void QtSizeFPropertyManager::setRange(QtProperty *property, const QSizeF &minimum, const QSizeF &maximum)

    Sets the range of valid values.

    This is a convenience function defining the range of valid values
    in one go; setting the \a minimum and \a maximum values for the
    given \a property with a single function call.

    When setting a new range, the current value is adjusted if
    necessary (ensuring that the value remains within the range).

    \sa setMinimum(), setMaximum(), rangeChanged()
*/
void QtSizeFPropertyManager::setRange(QtProperty *property, const QSizeF &_minVal, const QSizeF &_maxVal)
{
	auto it = d_ptr->m_values.find(property);
	if( it == d_ptr->m_values.end() ) {
		return;
	}

	auto minVal = _minVal;
	auto maxVal = _maxVal;

	orderSizeBorders(minVal, maxVal);

	auto& data = it.value();

	if( data.minimumValue() == minVal && data.maximumValue() == maxVal ) {
		return;
	}

	const auto oldMinVal = data.minVal;
	const auto oldMaxVal = data.maxVal;
	const auto oldVal = data.val;

	data.setMinimumValue(minVal);
	data.setMaximumValue(maxVal);

	emit rangeChanged(property, data.minVal, data.maxVal);

	if( oldMinVal != data.minVal ) {
		emit attributeChanged(property, "minimum", data.minVal);
	}
	if( oldMaxVal != data.maxVal ) {
		emit attributeChanged(property, "maximum", data.maxVal);
	}
	if( data.val != oldVal ) {
		emit propertyChanged(property);
		emit valueChanged(property, data.val);
		emit QtAbstractPropertyManager::valueChanged(property, data.val);
	}
}

/*!
    \reimp
*/
void QtSizeFPropertyManager::initializeProperty(QtProperty *property)
{
    d_ptr->m_values[property] = QtSizeFPropertyManagerPrivate::Data();

    QtProperty *wProp = d_ptr->m_doublePropertyManager->addProperty();
    wProp->setPropertyName(tr("Width"));
    d_ptr->m_doublePropertyManager->setDecimals(wProp, decimals(property));
    d_ptr->m_doublePropertyManager->setValue(wProp, 0);
    d_ptr->m_doublePropertyManager->setMinimum(wProp, 0);
    d_ptr->m_propertyToW[property] = wProp;
    d_ptr->m_wToProperty[wProp] = property;
    property->addSubProperty(wProp);

    QtProperty *hProp = d_ptr->m_doublePropertyManager->addProperty();
    hProp->setPropertyName(tr("Height"));
    d_ptr->m_doublePropertyManager->setDecimals(hProp, decimals(property));
    d_ptr->m_doublePropertyManager->setValue(hProp, 0);
    d_ptr->m_doublePropertyManager->setMinimum(hProp, 0);
    d_ptr->m_propertyToH[property] = hProp;
    d_ptr->m_hToProperty[hProp] = property;
    property->addSubProperty(hProp);
}

/*!
    \reimp
*/
void QtSizeFPropertyManager::uninitializeProperty(QtProperty *property)
{
    QtProperty *wProp = d_ptr->m_propertyToW[property];
    if (wProp) {
        d_ptr->m_wToProperty.remove(wProp);
        delete wProp;
    }
    d_ptr->m_propertyToW.remove(property);

    QtProperty *hProp = d_ptr->m_propertyToH[property];
    if (hProp) {
        d_ptr->m_hToProperty.remove(hProp);
        delete hProp;
    }
    d_ptr->m_propertyToH.remove(property);

    d_ptr->m_values.remove(property);
}

// QtRectPropertyManager

void QtRectPropertyManagerPrivate::slotIntChanged(QtProperty *property, int value)
{
    if (QtProperty *prop = m_xToProperty.value(property, 0)) {
        QRect r = m_values[prop].val;
        r.moveLeft(value);
        q_ptr->setValue(prop, r);
    } else if (QtProperty *prop = m_yToProperty.value(property)) {
        QRect r = m_values[prop].val;
        r.moveTop(value);
        q_ptr->setValue(prop, r);
    } else if (QtProperty *prop = m_wToProperty.value(property, 0)) {
        Data data = m_values[prop];
        QRect r = data.val;
        r.setWidth(value);
        if (!data.constraint.isNull() && data.constraint.x() + data.constraint.width() < r.x() + r.width()) {
            r.moveLeft(data.constraint.left() + data.constraint.width() - r.width());
        }
        q_ptr->setValue(prop, r);
    } else if (QtProperty *prop = m_hToProperty.value(property, 0)) {
        Data data = m_values[prop];
        QRect r = data.val;
        r.setHeight(value);
        if (!data.constraint.isNull() && data.constraint.y() + data.constraint.height() < r.y() + r.height()) {
            r.moveTop(data.constraint.top() + data.constraint.height() - r.height());
        }
        q_ptr->setValue(prop, r);
    }
}

void QtRectPropertyManagerPrivate::slotPropertyDestroyed(QtProperty *property)
{
    if (QtProperty *pointProp = m_xToProperty.value(property, 0)) {
        m_propertyToX[pointProp] = 0;
        m_xToProperty.remove(property);
    } else if (QtProperty *pointProp = m_yToProperty.value(property, 0)) {
        m_propertyToY[pointProp] = 0;
        m_yToProperty.remove(property);
    } else if (QtProperty *pointProp = m_wToProperty.value(property, 0)) {
        m_propertyToW[pointProp] = 0;
        m_wToProperty.remove(property);
    } else if (QtProperty *pointProp = m_hToProperty.value(property, 0)) {
        m_propertyToH[pointProp] = 0;
        m_hToProperty.remove(property);
    }
}

void QtRectPropertyManagerPrivate::setConstraint(QtProperty *property,
            const QRect &constraint, const QRect &val)
{
    const bool isNull = constraint.isNull();
    const int left   = isNull ? INT_MIN : constraint.left();
    const int right  = isNull ? INT_MAX : constraint.left() + constraint.width();
    const int top    = isNull ? INT_MIN : constraint.top();
    const int bottom = isNull ? INT_MAX : constraint.top() + constraint.height();
    const int width  = isNull ? INT_MAX : constraint.width();
    const int height = isNull ? INT_MAX : constraint.height();

    m_intPropertyManager->setRange(m_propertyToX[property], left, right);
    m_intPropertyManager->setRange(m_propertyToY[property], top, bottom);
    m_intPropertyManager->setRange(m_propertyToW[property], 0, width);
    m_intPropertyManager->setRange(m_propertyToH[property], 0, height);

    m_intPropertyManager->setValue(m_propertyToX[property], val.x());
    m_intPropertyManager->setValue(m_propertyToY[property], val.y());
    m_intPropertyManager->setValue(m_propertyToW[property], val.width());
    m_intPropertyManager->setValue(m_propertyToH[property], val.height());
}

/*!
    \class QtRectPropertyManager

    \brief The QtRectPropertyManager provides and manages QRect properties.

    A rectangle property has nested \e x, \e y, \e width and \e height
    subproperties. The top-level property's value can be retrieved
    using the value() function, and set using the setValue() slot.

    The subproperties are created by a QtIntPropertyManager object. This
    manager can be retrieved using the subIntPropertyManager() function. In
    order to provide editing widgets for the subproperties in a
    property browser widget, this manager must be associated with an
    editor factory.

    A rectangle property also has a constraint rectangle which can be
    retrieved using the constraint() function, and set using the
    setConstraint() slot.

    In addition, QtRectPropertyManager provides the valueChanged() signal
    which is emitted whenever a property created by this manager
    changes, and the constraintChanged() signal which is emitted
    whenever such a property changes its constraint rectangle.

    \sa QtAbstractPropertyManager, QtIntPropertyManager, QtRectFPropertyManager
*/

/*!
    \fn void QtRectPropertyManager::valueChanged(QtProperty *property, const QRect &value)

    This signal is emitted whenever a property created by this manager
    changes its value, passing a pointer to the \a property and the new
    \a value as parameters.

    \sa setValue()
*/

/*!
    \fn void QtRectPropertyManager::constraintChanged(QtProperty *property, const QRect &constraint)

    This signal is emitted whenever property changes its constraint
    rectangle, passing a pointer to the \a property and the new \a
    constraint rectangle as parameters.

    \sa setConstraint()
*/

/*!
    Creates a manager with the given \a parent.
*/
QtRectPropertyManager::QtRectPropertyManager(QObject *parent)
    : QtAbstractPropertyManager(parent)
{
    d_ptr = new QtRectPropertyManagerPrivate;
    d_ptr->q_ptr = this;

    d_ptr->m_intPropertyManager = new QtIntPropertyManager(this);
    connect(d_ptr->m_intPropertyManager, SIGNAL(valueChanged(QtProperty *, int)),
                this, SLOT(slotIntChanged(QtProperty *, int)));
    connect(d_ptr->m_intPropertyManager, SIGNAL(propertyDestroyed(QtProperty *)),
                this, SLOT(slotPropertyDestroyed(QtProperty *)));
}

/*!
    Destroys this manager, and all the properties it has created.
*/
QtRectPropertyManager::~QtRectPropertyManager()
{
    clear();
    delete d_ptr;
}

/*!
    Returns the manager that creates the nested \e x, \e y, \e width
    and \e height subproperties.

    In order to provide editing widgets for the mentioned
    subproperties in a property browser widget, this manager must be
    associated with an editor factory.

    \sa QtAbstractPropertyBrowser::setFactoryForManager()
*/
QtIntPropertyManager *QtRectPropertyManager::subIntPropertyManager() const
{
    return d_ptr->m_intPropertyManager;
}

/*!
    Returns the given \a property's value.

    If the given \a property is not managed by this manager, this
    function returns an invalid rectangle.

    \sa setValue(), constraint()
*/
QRect QtRectPropertyManager::value(const QtProperty *property) const
{
    return getValue<QRect>(d_ptr->m_values, property);
}


QVariant QtRectPropertyManager::variantValue(const QtProperty *property) const
{
	return QVariant::fromValue(value(property));
}


QVariant QtRectPropertyManager::attributeValue(const QtProperty *property, const QString &attribute) const
{
	if( attribute == "constraint" ) {
		return QVariant(constraint(property));
	} else {
		qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	}
	return QVariant();
}


int QtRectPropertyManager::propertyTypeId() const
{
	return QVariant::Rect;
}


QList<QtAbstractPropertyManager*> QtRectPropertyManager::subPropertyManagers() const
{
	return{ subIntPropertyManager() };
}


/*!
    Returns the given \a property's constraining rectangle. If returned value is null QRect it means there is no constraint applied.

    \sa value(), setConstraint()
*/
QRect QtRectPropertyManager::constraint(const QtProperty *property) const
{
    return getData<QRect>(d_ptr->m_values, &QtRectPropertyManagerPrivate::Data::constraint, property, QRect());
}

/*!
    \reimp
*/
QString QtRectPropertyManager::valueText(const QtProperty *property) const
{
    const QtRectPropertyManagerPrivate::PropertyValueMap::const_iterator it = d_ptr->m_values.constFind(property);
    if (it == d_ptr->m_values.constEnd())
        return QString();
    const QRect v = it.value().val;
    return QString(tr("[(%1, %2), %3 x %4]").arg(QString::number(v.x()))
                                .arg(QString::number(v.y()))
                                .arg(QString::number(v.width()))
                                .arg(QString::number(v.height())));
}

/*!
    \fn void QtRectPropertyManager::setValue(QtProperty *property, const QRect &value)

    Sets the value of the given \a property to \a value. Nested
    properties are updated automatically.

    If the specified \a value is not inside the given \a property's
    constraining rectangle, the value is adjusted accordingly to fit
    within the constraint.

    \sa value(), setConstraint(), valueChanged()
*/
void QtRectPropertyManager::setValue(QtProperty *property, const QRect &val)
{
    const QtRectPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    QtRectPropertyManagerPrivate::Data data = it.value();

    QRect newRect = val.normalized();
    if (!data.constraint.isNull() && !data.constraint.contains(newRect)) {
        const QRect r1 = data.constraint;
        const QRect r2 = newRect;
        newRect.setLeft(qMax(r1.left(), r2.left()));
        newRect.setRight(qMin(r1.right(), r2.right()));
        newRect.setTop(qMax(r1.top(), r2.top()));
        newRect.setBottom(qMin(r1.bottom(), r2.bottom()));
        if (newRect.width() < 0 || newRect.height() < 0)
            return;
    }

    if (data.val == newRect)
        return;

    data.val = newRect;

    it.value() = data;
    d_ptr->m_intPropertyManager->setValue(d_ptr->m_propertyToX[property], newRect.x());
    d_ptr->m_intPropertyManager->setValue(d_ptr->m_propertyToY[property], newRect.y());
    d_ptr->m_intPropertyManager->setValue(d_ptr->m_propertyToW[property], newRect.width());
    d_ptr->m_intPropertyManager->setValue(d_ptr->m_propertyToH[property], newRect.height());

    emit propertyChanged(property);
    emit valueChanged(property, data.val);
	emit QtAbstractPropertyManager::valueChanged(property, QVariant(data.val));
}


void QtRectPropertyManager::setValue(QtProperty *property, const QVariant &val)
{
	Q_ASSERT(val.canConvert<QRect>());
	auto s_val = qvariant_cast<QRect>(val);
	setValue(property, s_val);
}


void QtRectPropertyManager::setAttribute(QtProperty *property, const QString &attribute, const QVariant &value)
{
	if( attribute == "constraint" ) {
		setConstraint(property, qvariant_cast<QRect>(value));
	} else {
		qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	}
}


/*!
    Sets the given \a property's constraining rectangle to \a
    constraint.

    When setting the constraint, the current value is adjusted if
    necessary (ensuring that the current rectangle value is inside the
    constraint). In order to reset the constraint pass a null QRect value.

    \sa setValue(), constraint(), constraintChanged()
*/
void QtRectPropertyManager::setConstraint(QtProperty *property, const QRect &constraint)
{
    const QtRectPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    QtRectPropertyManagerPrivate::Data data = it.value();

    QRect newConstraint = constraint.normalized();
    if (data.constraint == newConstraint)
        return;

    const QRect oldVal = data.val;

    data.constraint = newConstraint;

    if (!data.constraint.isNull() && !data.constraint.contains(oldVal)) {
        QRect r1 = data.constraint;
        QRect r2 = data.val;

        if (r2.width() > r1.width())
            r2.setWidth(r1.width());
        if (r2.height() > r1.height())
            r2.setHeight(r1.height());
        if (r2.left() < r1.left())
            r2.moveLeft(r1.left());
        else if (r2.right() > r1.right())
            r2.moveRight(r1.right());
        if (r2.top() < r1.top())
            r2.moveTop(r1.top());
        else if (r2.bottom() > r1.bottom())
            r2.moveBottom(r1.bottom());

        data.val = r2;
    }

    it.value() = data;

    emit constraintChanged(property, data.constraint);
	emit attributeChanged(property, "constraint", data.constraint);

    d_ptr->setConstraint(property, data.constraint, data.val);

    if (data.val == oldVal)
        return;

    emit propertyChanged(property);
    emit valueChanged(property, data.val);
	emit QtAbstractPropertyManager::valueChanged(property, data.val);
}

/*!
    \reimp
*/
void QtRectPropertyManager::initializeProperty(QtProperty *property)
{
    d_ptr->m_values[property] = QtRectPropertyManagerPrivate::Data();

    QtProperty *xProp = d_ptr->m_intPropertyManager->addProperty();
    xProp->setPropertyName(tr("X"));
    d_ptr->m_intPropertyManager->setValue(xProp, 0);
    d_ptr->m_propertyToX[property] = xProp;
    d_ptr->m_xToProperty[xProp] = property;
    property->addSubProperty(xProp);

    QtProperty *yProp = d_ptr->m_intPropertyManager->addProperty();
    yProp->setPropertyName(tr("Y"));
    d_ptr->m_intPropertyManager->setValue(yProp, 0);
    d_ptr->m_propertyToY[property] = yProp;
    d_ptr->m_yToProperty[yProp] = property;
    property->addSubProperty(yProp);

    QtProperty *wProp = d_ptr->m_intPropertyManager->addProperty();
    wProp->setPropertyName(tr("Width"));
    d_ptr->m_intPropertyManager->setValue(wProp, 0);
    d_ptr->m_intPropertyManager->setMinimum(wProp, 0);
    d_ptr->m_propertyToW[property] = wProp;
    d_ptr->m_wToProperty[wProp] = property;
    property->addSubProperty(wProp);

    QtProperty *hProp = d_ptr->m_intPropertyManager->addProperty();
    hProp->setPropertyName(tr("Height"));
    d_ptr->m_intPropertyManager->setValue(hProp, 0);
    d_ptr->m_intPropertyManager->setMinimum(hProp, 0);
    d_ptr->m_propertyToH[property] = hProp;
    d_ptr->m_hToProperty[hProp] = property;
    property->addSubProperty(hProp);
}

/*!
    \reimp
*/
void QtRectPropertyManager::uninitializeProperty(QtProperty *property)
{
    QtProperty *xProp = d_ptr->m_propertyToX[property];
    if (xProp) {
        d_ptr->m_xToProperty.remove(xProp);
        delete xProp;
    }
    d_ptr->m_propertyToX.remove(property);

    QtProperty *yProp = d_ptr->m_propertyToY[property];
    if (yProp) {
        d_ptr->m_yToProperty.remove(yProp);
        delete yProp;
    }
    d_ptr->m_propertyToY.remove(property);

    QtProperty *wProp = d_ptr->m_propertyToW[property];
    if (wProp) {
        d_ptr->m_wToProperty.remove(wProp);
        delete wProp;
    }
    d_ptr->m_propertyToW.remove(property);

    QtProperty *hProp = d_ptr->m_propertyToH[property];
    if (hProp) {
        d_ptr->m_hToProperty.remove(hProp);
        delete hProp;
    }
    d_ptr->m_propertyToH.remove(property);

    d_ptr->m_values.remove(property);
}

// QtRectFPropertyManager

void QtRectFPropertyManagerPrivate::slotDoubleChanged(QtProperty *property, double value)
{
    if (QtProperty *prop = m_xToProperty.value(property, 0)) {
        QRectF r = m_values[prop].val;
        r.moveLeft(value);
        q_ptr->setValue(prop, r);
    } else if (QtProperty *prop = m_yToProperty.value(property, 0)) {
        QRectF r = m_values[prop].val;
        r.moveTop(value);
        q_ptr->setValue(prop, r);
    } else if (QtProperty *prop = m_wToProperty.value(property, 0)) {
        Data data = m_values[prop];
        QRectF r = data.val;
        r.setWidth(value);
        if (!data.constraint.isNull() && data.constraint.x() + data.constraint.width() < r.x() + r.width()) {
            r.moveLeft(data.constraint.left() + data.constraint.width() - r.width());
        }
        q_ptr->setValue(prop, r);
    } else if (QtProperty *prop = m_hToProperty.value(property, 0)) {
        Data data = m_values[prop];
        QRectF r = data.val;
        r.setHeight(value);
        if (!data.constraint.isNull() && data.constraint.y() + data.constraint.height() < r.y() + r.height()) {
            r.moveTop(data.constraint.top() + data.constraint.height() - r.height());
        }
        q_ptr->setValue(prop, r);
    }
}

void QtRectFPropertyManagerPrivate::slotPropertyDestroyed(QtProperty *property)
{
    if (QtProperty *pointProp = m_xToProperty.value(property, 0)) {
        m_propertyToX[pointProp] = 0;
        m_xToProperty.remove(property);
    } else if (QtProperty *pointProp = m_yToProperty.value(property, 0)) {
        m_propertyToY[pointProp] = 0;
        m_yToProperty.remove(property);
    } else if (QtProperty *pointProp = m_wToProperty.value(property, 0)) {
        m_propertyToW[pointProp] = 0;
        m_wToProperty.remove(property);
    } else if (QtProperty *pointProp = m_hToProperty.value(property, 0)) {
        m_propertyToH[pointProp] = 0;
        m_hToProperty.remove(property);
    }
}

void QtRectFPropertyManagerPrivate::setConstraint(QtProperty *property,
            const QRectF &constraint, const QRectF &val)
{
    const bool isNull = constraint.isNull();
    const float left   = isNull ? FLT_MIN : constraint.left();
    const float right  = isNull ? FLT_MAX : constraint.left() + constraint.width();
    const float top    = isNull ? FLT_MIN : constraint.top();
    const float bottom = isNull ? FLT_MAX : constraint.top() + constraint.height();
    const float width  = isNull ? FLT_MAX : constraint.width();
    const float height = isNull ? FLT_MAX : constraint.height();

    m_doublePropertyManager->setRange(m_propertyToX[property], left, right);
    m_doublePropertyManager->setRange(m_propertyToY[property], top, bottom);
    m_doublePropertyManager->setRange(m_propertyToW[property], 0, width);
    m_doublePropertyManager->setRange(m_propertyToH[property], 0, height);

    m_doublePropertyManager->setValue(m_propertyToX[property], val.x());
    m_doublePropertyManager->setValue(m_propertyToY[property], val.y());
    m_doublePropertyManager->setValue(m_propertyToW[property], val.width());
    m_doublePropertyManager->setValue(m_propertyToH[property], val.height());
}

/*!
    \class QtRectFPropertyManager

    \brief The QtRectFPropertyManager provides and manages QRectF properties.

    A rectangle property has nested \e x, \e y, \e width and \e height
    subproperties. The top-level property's value can be retrieved
    using the value() function, and set using the setValue() slot.

    The subproperties are created by a QtDoublePropertyManager object. This
    manager can be retrieved using the subDoublePropertyManager() function. In
    order to provide editing widgets for the subproperties in a
    property browser widget, this manager must be associated with an
    editor factory.

    A rectangle property also has a constraint rectangle which can be
    retrieved using the constraint() function, and set using the
    setConstraint() slot.

    In addition, QtRectFPropertyManager provides the valueChanged() signal
    which is emitted whenever a property created by this manager
    changes, and the constraintChanged() signal which is emitted
    whenever such a property changes its constraint rectangle.

    \sa QtAbstractPropertyManager, QtDoublePropertyManager, QtRectPropertyManager
*/

/*!
    \fn void QtRectFPropertyManager::valueChanged(QtProperty *property, const QRectF &value)

    This signal is emitted whenever a property created by this manager
    changes its value, passing a pointer to the \a property and the new
    \a value as parameters.

    \sa setValue()
*/

/*!
    \fn void QtRectFPropertyManager::constraintChanged(QtProperty *property, const QRectF &constraint)

    This signal is emitted whenever property changes its constraint
    rectangle, passing a pointer to the \a property and the new \a
    constraint rectangle as parameters.

    \sa setConstraint()
*/

/*!
    \fn void QtRectFPropertyManager::decimalsChanged(QtProperty *property, int prec)

    This signal is emitted whenever a property created by this manager
    changes its precision of value, passing a pointer to the
    \a property and the new \a prec value

    \sa setDecimals()
*/

/*!
    Creates a manager with the given \a parent.
*/
QtRectFPropertyManager::QtRectFPropertyManager(QObject *parent)
    : QtAbstractPropertyManager(parent)
{
    d_ptr = new QtRectFPropertyManagerPrivate;
    d_ptr->q_ptr = this;

    d_ptr->m_doublePropertyManager = new QtDoublePropertyManager(this);
    connect(d_ptr->m_doublePropertyManager, SIGNAL(valueChanged(QtProperty *, double)),
                this, SLOT(slotDoubleChanged(QtProperty *, double)));
    connect(d_ptr->m_doublePropertyManager, SIGNAL(propertyDestroyed(QtProperty *)),
                this, SLOT(slotPropertyDestroyed(QtProperty *)));
}

/*!
    Destroys this manager, and all the properties it has created.
*/
QtRectFPropertyManager::~QtRectFPropertyManager()
{
    clear();
    delete d_ptr;
}

/*!
    Returns the manager that creates the nested \e x, \e y, \e width
    and \e height subproperties.

    In order to provide editing widgets for the mentioned
    subproperties in a property browser widget, this manager must be
    associated with an editor factory.

    \sa QtAbstractPropertyBrowser::setFactoryForManager()
*/
QtDoublePropertyManager *QtRectFPropertyManager::subDoublePropertyManager() const
{
    return d_ptr->m_doublePropertyManager;
}

/*!
    Returns the given \a property's value.

    If the given \a property is not managed by this manager, this
    function returns an invalid rectangle.

    \sa setValue(), constraint()
*/
QRectF QtRectFPropertyManager::value(const QtProperty *property) const
{
    return getValue<QRectF>(d_ptr->m_values, property);
}


QVariant QtRectFPropertyManager::variantValue(const QtProperty *property) const
{
	return QVariant::fromValue(value(property));
}


/*!
    Returns the given \a property's precision, in decimals.

    \sa setDecimals()
*/
int QtRectFPropertyManager::decimals(const QtProperty *property) const
{
    return getData<int>(d_ptr->m_values, &QtRectFPropertyManagerPrivate::Data::decimals, property, 0);
}


QVariant QtRectFPropertyManager::attributeValue(const QtProperty *property, const QString &attribute) const
{
	if( attribute == "constraint" ) {
		return QVariant(constraint(property));
	} else if( attribute == "decimals" ) {
		return QVariant(decimals(property));
	} else {
		qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	}
	return QVariant();
}


int QtRectFPropertyManager::propertyTypeId() const
{
	return QVariant::RectF;
}


QList<QtAbstractPropertyManager*> QtRectFPropertyManager::subPropertyManagers() const
{
	return{ subDoublePropertyManager() };
}


/*!
    Returns the given \a property's constraining rectangle. If returned value is null QRectF it means there is no constraint applied.

    \sa value(), setConstraint()
*/
QRectF QtRectFPropertyManager::constraint(const QtProperty *property) const
{
    return getData<QRectF>(d_ptr->m_values, &QtRectFPropertyManagerPrivate::Data::constraint, property, QRect());
}

/*!
    \reimp
*/
QString QtRectFPropertyManager::valueText(const QtProperty *property) const
{
    const QtRectFPropertyManagerPrivate::PropertyValueMap::const_iterator it = d_ptr->m_values.constFind(property);
    if (it == d_ptr->m_values.constEnd())
        return QString();
    const QRectF v = it.value().val;
    const int dec = it.value().decimals;
    return QString(tr("[(%1, %2), %3 x %4]").arg(QString::number(v.x(), 'f', dec))
                                .arg(QString::number(v.y(), 'f', dec))
                                .arg(QString::number(v.width(), 'f', dec))
                                .arg(QString::number(v.height(), 'f', dec)));
}

/*!
    \fn void QtRectFPropertyManager::setValue(QtProperty *property, const QRectF &value)

    Sets the value of the given \a property to \a value. Nested
    properties are updated automatically.

    If the specified \a value is not inside the given \a property's
    constraining rectangle, the value is adjusted accordingly to fit
    within the constraint.

    \sa value(), setConstraint(), valueChanged()
*/
void QtRectFPropertyManager::setValue(QtProperty *property, const QRectF &val)
{
    const QtRectFPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    QtRectFPropertyManagerPrivate::Data data = it.value();

    QRectF newRect = val.normalized();
    if (!data.constraint.isNull() && !data.constraint.contains(newRect)) {
        const QRectF r1 = data.constraint;
        const QRectF r2 = newRect;
        newRect.setLeft(qMax(r1.left(), r2.left()));
        newRect.setRight(qMin(r1.right(), r2.right()));
        newRect.setTop(qMax(r1.top(), r2.top()));
        newRect.setBottom(qMin(r1.bottom(), r2.bottom()));
        if (newRect.width() < 0 || newRect.height() < 0)
            return;
    }

    if (data.val == newRect)
        return;

    data.val = newRect;

    it.value() = data;
    d_ptr->m_doublePropertyManager->setValue(d_ptr->m_propertyToX[property], newRect.x());
    d_ptr->m_doublePropertyManager->setValue(d_ptr->m_propertyToY[property], newRect.y());
    d_ptr->m_doublePropertyManager->setValue(d_ptr->m_propertyToW[property], newRect.width());
    d_ptr->m_doublePropertyManager->setValue(d_ptr->m_propertyToH[property], newRect.height());

    emit propertyChanged(property);
    emit valueChanged(property, data.val);
	emit QtAbstractPropertyManager::valueChanged(property, QVariant(data.val));
}


void QtRectFPropertyManager::setValue(QtProperty *property, const QVariant &val)
{
	Q_ASSERT(val.canConvert<QRectF>());
	auto s_val = qvariant_cast<QRectF>(val);
	setValue(property, s_val);
}


void QtRectFPropertyManager::setAttribute(QtProperty *property, const QString &attribute, const QVariant &value)
{
	if( attribute == "constraint" ) {
		setConstraint(property, qvariant_cast<QRectF>(value));
	} else if( attribute == "decimals" ) {
		setDecimals(property, qvariant_cast<int>(value));
	} else {
		qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	}
}


/*!
    Sets the given \a property's constraining rectangle to \a
    constraint.

    When setting the constraint, the current value is adjusted if
    necessary (ensuring that the current rectangle value is inside the
    constraint). In order to reset the constraint pass a null QRectF value.

    \sa setValue(), constraint(), constraintChanged()
*/
void QtRectFPropertyManager::setConstraint(QtProperty *property, const QRectF &constraint)
{
    const QtRectFPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    QtRectFPropertyManagerPrivate::Data data = it.value();

    QRectF newConstraint = constraint.normalized();
    if (data.constraint == newConstraint)
        return;

    const QRectF oldVal = data.val;

    data.constraint = newConstraint;

    if (!data.constraint.isNull() && !data.constraint.contains(oldVal)) {
        QRectF r1 = data.constraint;
        QRectF r2 = data.val;

        if (r2.width() > r1.width())
            r2.setWidth(r1.width());
        if (r2.height() > r1.height())
            r2.setHeight(r1.height());
        if (r2.left() < r1.left())
            r2.moveLeft(r1.left());
        else if (r2.right() > r1.right())
            r2.moveRight(r1.right());
        if (r2.top() < r1.top())
            r2.moveTop(r1.top());
        else if (r2.bottom() > r1.bottom())
            r2.moveBottom(r1.bottom());

        data.val = r2;
    }

    it.value() = data;

    emit constraintChanged(property, data.constraint);
	emit attributeChanged(property, "constraint", data.constraint);

    d_ptr->setConstraint(property, data.constraint, data.val);

    if (data.val == oldVal)
        return;

    emit propertyChanged(property);
    emit valueChanged(property, data.val);
	emit QtAbstractPropertyManager::valueChanged(property, data.val);
}

/*!
    \fn void QtRectFPropertyManager::setDecimals(QtProperty *property, int prec)

    Sets the precision of the given \a property to \a prec.

    The valid decimal range is 0-13. The default is 2.

    \sa decimals()
*/
void QtRectFPropertyManager::setDecimals(QtProperty *property, int prec)
{
    const QtRectFPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    QtRectFPropertyManagerPrivate::Data data = it.value();

    if (prec > 13)
        prec = 13;
    else if (prec < 0)
        prec = 0;

    if (data.decimals == prec)
        return;

    data.decimals = prec;
    d_ptr->m_doublePropertyManager->setDecimals(d_ptr->m_propertyToX[property], prec);
    d_ptr->m_doublePropertyManager->setDecimals(d_ptr->m_propertyToY[property], prec);
    d_ptr->m_doublePropertyManager->setDecimals(d_ptr->m_propertyToW[property], prec);
    d_ptr->m_doublePropertyManager->setDecimals(d_ptr->m_propertyToH[property], prec);

    it.value() = data;

    emit decimalsChanged(property, data.decimals);
	emit attributeChanged(property, "decimals", data.decimals);
}

/*!
    \reimp
*/
void QtRectFPropertyManager::initializeProperty(QtProperty *property)
{
    d_ptr->m_values[property] = QtRectFPropertyManagerPrivate::Data();

    QtProperty *xProp = d_ptr->m_doublePropertyManager->addProperty();
    xProp->setPropertyName(tr("X"));
    d_ptr->m_doublePropertyManager->setDecimals(xProp, decimals(property));
    d_ptr->m_doublePropertyManager->setValue(xProp, 0);
    d_ptr->m_propertyToX[property] = xProp;
    d_ptr->m_xToProperty[xProp] = property;
    property->addSubProperty(xProp);

    QtProperty *yProp = d_ptr->m_doublePropertyManager->addProperty();
    yProp->setPropertyName(tr("Y"));
    d_ptr->m_doublePropertyManager->setDecimals(yProp, decimals(property));
    d_ptr->m_doublePropertyManager->setValue(yProp, 0);
    d_ptr->m_propertyToY[property] = yProp;
    d_ptr->m_yToProperty[yProp] = property;
    property->addSubProperty(yProp);

    QtProperty *wProp = d_ptr->m_doublePropertyManager->addProperty();
    wProp->setPropertyName(tr("Width"));
    d_ptr->m_doublePropertyManager->setDecimals(wProp, decimals(property));
    d_ptr->m_doublePropertyManager->setValue(wProp, 0);
    d_ptr->m_doublePropertyManager->setMinimum(wProp, 0);
    d_ptr->m_propertyToW[property] = wProp;
    d_ptr->m_wToProperty[wProp] = property;
    property->addSubProperty(wProp);

    QtProperty *hProp = d_ptr->m_doublePropertyManager->addProperty();
    hProp->setPropertyName(tr("Height"));
    d_ptr->m_doublePropertyManager->setDecimals(hProp, decimals(property));
    d_ptr->m_doublePropertyManager->setValue(hProp, 0);
    d_ptr->m_doublePropertyManager->setMinimum(hProp, 0);
    d_ptr->m_propertyToH[property] = hProp;
    d_ptr->m_hToProperty[hProp] = property;
    property->addSubProperty(hProp);
}

/*!
    \reimp
*/
void QtRectFPropertyManager::uninitializeProperty(QtProperty *property)
{
    QtProperty *xProp = d_ptr->m_propertyToX[property];
    if (xProp) {
        d_ptr->m_xToProperty.remove(xProp);
        delete xProp;
    }
    d_ptr->m_propertyToX.remove(property);

    QtProperty *yProp = d_ptr->m_propertyToY[property];
    if (yProp) {
        d_ptr->m_yToProperty.remove(yProp);
        delete yProp;
    }
    d_ptr->m_propertyToY.remove(property);

    QtProperty *wProp = d_ptr->m_propertyToW[property];
    if (wProp) {
        d_ptr->m_wToProperty.remove(wProp);
        delete wProp;
    }
    d_ptr->m_propertyToW.remove(property);

    QtProperty *hProp = d_ptr->m_propertyToH[property];
    if (hProp) {
        d_ptr->m_hToProperty.remove(hProp);
        delete hProp;
    }
    d_ptr->m_propertyToH.remove(property);

    d_ptr->m_values.remove(property);
}

// QtEnumPropertyManager

/*!
    \class QtEnumPropertyManager

    \brief The QtEnumPropertyManager provides and manages enum properties.

    Each enum property has an associated list of enum names which can
    be retrieved using the enumNames() function, and set using the
    corresponding setEnumNames() function. An enum property's value is
    represented by an index in this list, and can be retrieved and set
    using the value() and setValue() slots respectively.

    Each enum value can also have an associated icon. The mapping from
    values to icons can be set using the setEnumIcons() function and
    queried with the enumIcons() function.

    In addition, QtEnumPropertyManager provides the valueChanged() signal
    which is emitted whenever a property created by this manager
    changes. The enumNamesChanged() or enumIconsChanged() signal is emitted
    whenever the list of enum names or icons is altered.

    \sa QtAbstractPropertyManager, QtEnumEditorFactory
*/

/*!
    \fn void QtEnumPropertyManager::valueChanged(QtProperty *property, int value)

    This signal is emitted whenever a property created by this manager
    changes its value, passing a pointer to the \a property and the new
    \a value as parameters.

    \sa setValue()
*/

/*!
    \fn void QtEnumPropertyManager::enumNamesChanged(QtProperty *property, const QStringList &names)

    This signal is emitted whenever a property created by this manager
    changes its enum names, passing a pointer to the \a property and
    the new \a names as parameters.

    \sa setEnumNames()
*/

/*!
    \fn void QtEnumPropertyManager::enumIconsChanged(QtProperty *property, const QMap<int, QIcon> &icons)

    This signal is emitted whenever a property created by this manager
    changes its enum icons, passing a pointer to the \a property and
    the new mapping of values to \a icons as parameters.

    \sa setEnumIcons()
*/

/*!
    Creates a manager with the given \a parent.
*/
QtEnumPropertyManager::QtEnumPropertyManager(QObject *parent)
    : QtAbstractPropertyManager(parent)
{
    d_ptr = new QtEnumPropertyManagerPrivate;
    d_ptr->q_ptr = this;
}

/*!
    Destroys this manager, and all the properties it has created.
*/
QtEnumPropertyManager::~QtEnumPropertyManager()
{
    clear();
    delete d_ptr;
}

/*!
    Returns the given \a property's value which is an index in the
    list returned by enumNames()

    If the given property is not managed by this manager, this
    function returns -1.

    \sa enumNames(), setValue()
*/
int QtEnumPropertyManager::value(const QtProperty *property) const
{
    return getValue<int>(d_ptr->m_values, property, -1);
}


QVariant QtEnumPropertyManager::variantValue(const QtProperty *property) const
{
	return QVariant::fromValue(value(property));
}


QVariant QtEnumPropertyManager::attributeValue(const QtProperty *property, const QString &attribute) const
{
	if( attribute == "enumNames" ) {
		return QVariant(enumNames(property));
	} else if( attribute == "enumIcons" ) {
		QVariant v;
		qVariantSetValue(v, enumIcons(property));
		return v;
	} else if( attribute == "enumValues" ) {
		return QVariant(enumValues(property));
	} else {
		qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	}
	return QVariant();

}


int QtEnumPropertyManager::propertyTypeId() const
{
	return qMetaTypeId<QtEnumPropertyType>();
}

int QtEnumPropertyManager::valueTypeId() const
{
	return QVariant::Int;
}

/*!
    Returns the given \a property's list of enum names.

    \sa value(), setEnumNames()
*/
QStringList QtEnumPropertyManager::enumNames(const QtProperty *property) const
{
    return getData<QStringList>(d_ptr->m_values, &QtEnumPropertyManagerPrivate::Data::enumNames, property, QStringList());
}


QVariantList QtEnumPropertyManager::enumValues(const QtProperty *property) const
{
    return getData<QVariantList>(d_ptr->m_values, &QtEnumPropertyManagerPrivate::Data::enumValues, property, QVariantList());
}


/*!
    Returns the given \a property's map of enum values to their icons.

    \sa value(), setEnumIcons()
*/
QMap<int, QIcon> QtEnumPropertyManager::enumIcons(const QtProperty *property) const
{
    return getData<QMap<int, QIcon> >(d_ptr->m_values, &QtEnumPropertyManagerPrivate::Data::enumIcons, property, QMap<int, QIcon>());
}

/*!
    \reimp
*/
QString QtEnumPropertyManager::valueText(const QtProperty *property) const
{
    const QtEnumPropertyManagerPrivate::PropertyValueMap::const_iterator it = d_ptr->m_values.constFind(property);
    if (it == d_ptr->m_values.constEnd())
        return QString();

    const QtEnumPropertyManagerPrivate::Data &data = it.value();

    const int v = data.val;
    if (v >= 0 && v < data.enumNames.count())
        return data.enumNames.at(v);
    return QString();
}

/*!
    \reimp
*/
QIcon QtEnumPropertyManager::valueIcon(const QtProperty *property) const
{
    const QtEnumPropertyManagerPrivate::PropertyValueMap::const_iterator it = d_ptr->m_values.constFind(property);
    if (it == d_ptr->m_values.constEnd())
        return QIcon();

    const QtEnumPropertyManagerPrivate::Data &data = it.value();

    const int v = data.val;
    return data.enumIcons.value(v);
}

/*!
    \fn void QtEnumPropertyManager::setValue(QtProperty *property, int value)

    Sets the value of the given  \a property to \a value.

    The specified \a value must be less than the size of the given \a
    property's enumNames() list, and larger than (or equal to) 0.

    \sa value(), valueChanged()
*/
void QtEnumPropertyManager::setValue(QtProperty *property, int val)
{
    const QtEnumPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    QtEnumPropertyManagerPrivate::Data data = it.value();

    if (val >= data.enumNames.count())
        return;

    if (val < 0 && data.enumNames.count() > 0)
        return;

    if (val < 0)
        val = -1;

    if (data.val == val)
        return;

    data.val = val;

    it.value() = data;

    emit propertyChanged(property);
    emit valueChanged(property, data.val);
	emit QtAbstractPropertyManager::valueChanged(property, QVariant(data.val));
}


void QtEnumPropertyManager::setValue(QtProperty *property, const QVariant &val)
{
	Q_ASSERT(val.canConvert<int>());
	auto s_val = qvariant_cast<int>(val);
	setValue(property, s_val);
}


void QtEnumPropertyManager::setAttribute(QtProperty *property, const QString &attribute, const QVariant &value)
{
	if( attribute == "enumNames" ) {
		setEnumNames(property, qvariant_cast<QStringList>(value));
	} else if( attribute == "enumIcons" ) {
		setEnumIcons(property, qvariant_cast<QtIconMap>(value));
	} else if( attribute == "enumValues" ) {
		setEnumValues(property, qvariant_cast<QVariantList>(value));
	} else {
		qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	}
}


/*!
    Sets the given \a property's list of enum names to \a
    enumNames. The \a property's current value is reset to 0
    indicating the first item of the list.

    If the specified \a enumNames list is empty, the \a property's
    current value is set to -1.

    \sa enumNames(), enumNamesChanged()
*/
void QtEnumPropertyManager::setEnumNames(QtProperty *property, const QStringList &enumNames)
{
    const QtEnumPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    QtEnumPropertyManagerPrivate::Data data = it.value();

	if( data.enumNames == enumNames ) {
		return;
	}

	auto                               old_val  = data.val;
	QString                            old_name = data.enumNames.value(old_val, QString());
	bool                               value_chaged;

    data.enumNames = enumNames;

	QString                            new_name = data.enumNames.value(old_val, QString());
	
	if( !old_name.isNull() && old_name == new_name && old_val < enumNames.count() ) {
		// data.val unchanged
		value_chaged = false;
	} else if( old_val == -1 ) {
		value_chaged = false;
	} else {
		data.val = -1;
		value_chaged = true;
	}

    it.value() = data;

    emit enumNamesChanged(property, data.enumNames);
	emit attributeChanged(property, "enumNames", data.enumNames);
	emit propertyChanged(property);

	if( value_chaged ) {
		emit valueChanged(property, data.val);
		emit QtAbstractPropertyManager::valueChanged(property, QVariant(data.val));
	}
}


void QtEnumPropertyManager::setEnumValues(QtProperty *property, const QVariantList & enumValues)
{
    const QtEnumPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    QtEnumPropertyManagerPrivate::Data data = it.value();

	if( data.enumValues == enumValues ) {
		return;
	}

    data.enumValues = enumValues;

    it.value() = data;

    emit enumValuesChanged(property, data.enumValues);
	emit attributeChanged(property, "enumValues", data.enumValues);
    emit propertyChanged(property);
}


/*!
    Sets the given \a property's map of enum values to their icons to \a
    enumIcons.

    Each enum value can have associated icon. This association is represented with passed \a enumIcons map.

    \sa enumNames(), enumNamesChanged()
*/
void QtEnumPropertyManager::setEnumIcons(QtProperty *property, const QMap<int, QIcon> &enumIcons)
{
    const QtEnumPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    it.value().enumIcons = enumIcons;

    emit enumIconsChanged(property, it.value().enumIcons);

	QVariant v;
	qVariantSetValue(v, it.value().enumIcons);
	emit attributeChanged(property, "enumIcons", v);

    emit propertyChanged(property);
}

/*!
    \reimp
*/
void QtEnumPropertyManager::initializeProperty(QtProperty *property)
{
    d_ptr->m_values[property] = QtEnumPropertyManagerPrivate::Data();
}

/*!
    \reimp
*/
void QtEnumPropertyManager::uninitializeProperty(QtProperty *property)
{
    d_ptr->m_values.remove(property);
}

// QtFlagPropertyManager

void QtFlagPropertyManagerPrivate::slotBoolChanged(QtProperty *property, bool value)
{
    QtProperty *prop = m_flagToProperty.value(property, 0);
    if (prop == 0)
        return;

    QListIterator<QtProperty *> itProp(m_propertyToFlags[prop]);
    int level = 0;
    while (itProp.hasNext()) {
        QtProperty *p = itProp.next();
        if (p == property) {
            int v = m_values[prop].val;
            if (value) {
                v |= (1 << level);
            } else {
                v &= ~(1 << level);
            }
            q_ptr->setValue(prop, v);
            return;
        }
        level++;
    }
}

void QtFlagPropertyManagerPrivate::slotPropertyDestroyed(QtProperty *property)
{
    QtProperty *flagProperty = m_flagToProperty.value(property, 0);
    if (flagProperty == 0)
        return;

    m_propertyToFlags[flagProperty].replace(m_propertyToFlags[flagProperty].indexOf(property), 0);
    m_flagToProperty.remove(property);
}

/*!
    \class QtFlagPropertyManager

    \brief The QtFlagPropertyManager provides and manages flag properties.

    Each flag property has an associated list of flag names which can
    be retrieved using the flagNames() function, and set using the
    corresponding setFlagNames() function.

    The flag manager provides properties with nested boolean
    subproperties representing each flag, i.e. a flag property's value
    is the binary combination of the subproperties' values. A
    property's value can be retrieved and set using the value() and
    setValue() slots respectively. The combination of flags is represented
    by single int value - that's why it's possible to store up to
    32 independent flags in one flag property.

    The subproperties are created by a QtBoolPropertyManager object. This
    manager can be retrieved using the subBoolPropertyManager() function. In
    order to provide editing widgets for the subproperties in a
    property browser widget, this manager must be associated with an
    editor factory.

    In addition, QtFlagPropertyManager provides the valueChanged() signal
    which is emitted whenever a property created by this manager
    changes, and the flagNamesChanged() signal which is emitted
    whenever the list of flag names is altered.

    \sa QtAbstractPropertyManager, QtBoolPropertyManager
*/

/*!
    \fn void QtFlagPropertyManager::valueChanged(QtProperty *property, int value)

    This signal is emitted whenever a property created by this manager
    changes its value, passing a pointer to the \a  property and the new
    \a value as parameters.

    \sa setValue()
*/

/*!
    \fn void QtFlagPropertyManager::flagNamesChanged(QtProperty *property, const QStringList &names)

    This signal is emitted whenever a property created by this manager
    changes its flag names, passing a pointer to the \a property and the
    new \a names as parameters.

    \sa setFlagNames()
*/

/*!
    Creates a manager with the given \a parent.
*/
QtFlagPropertyManager::QtFlagPropertyManager(QObject *parent)
    : QtAbstractPropertyManager(parent)
{
    d_ptr = new QtFlagPropertyManagerPrivate;
    d_ptr->q_ptr = this;

    d_ptr->m_boolPropertyManager = new QtBoolPropertyManager(this);
    connect(d_ptr->m_boolPropertyManager, SIGNAL(valueChanged(QtProperty *, bool)),
                this, SLOT(slotBoolChanged(QtProperty *, bool)));
    connect(d_ptr->m_boolPropertyManager, SIGNAL(propertyDestroyed(QtProperty *)),
                this, SLOT(slotPropertyDestroyed(QtProperty *)));
}

/*!
    Destroys this manager, and all the properties it has created.
*/
QtFlagPropertyManager::~QtFlagPropertyManager()
{
    clear();
    delete d_ptr;
}

/*!
    Returns the manager that produces the nested boolean subproperties
    representing each flag.

    In order to provide editing widgets for the subproperties in a
    property browser widget, this manager must be associated with an
    editor factory.

    \sa QtAbstractPropertyBrowser::setFactoryForManager()
*/
QtBoolPropertyManager *QtFlagPropertyManager::subBoolPropertyManager() const
{
    return d_ptr->m_boolPropertyManager;
}

/*!
    Returns the given \a property's value.

    If the given property is not managed by this manager, this
    function returns 0.

    \sa flagNames(), setValue()
*/
int QtFlagPropertyManager::value(const QtProperty *property) const
{
    return getValue<int>(d_ptr->m_values, property, 0);
}


QVariant QtFlagPropertyManager::variantValue(const QtProperty *property) const
{
	return QVariant::fromValue(value(property));
}


QVariant QtFlagPropertyManager::attributeValue(const QtProperty *property, const QString &attribute) const
{
	if( attribute == "flagNames" ) {
		return QVariant(flagNames(property));
	} else {
		qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	}
	return QVariant();
}


int QtFlagPropertyManager::propertyTypeId() const
{
	return qMetaTypeId<QtFlagPropertyType>();
}

int QtFlagPropertyManager::valueTypeId() const
{
	return QVariant::Int;
}


QList<QtAbstractPropertyManager*> QtFlagPropertyManager::subPropertyManagers() const
{
	return{ subBoolPropertyManager() };
}


/*!
    Returns the given \a property's list of flag names.

    \sa value(), setFlagNames()
*/
QStringList QtFlagPropertyManager::flagNames(const QtProperty *property) const
{
    return getData<QStringList>(d_ptr->m_values, &QtFlagPropertyManagerPrivate::Data::flagNames, property, QStringList());
}

/*!
    \reimp
*/
QString QtFlagPropertyManager::valueText(const QtProperty *property) const
{
    const QtFlagPropertyManagerPrivate::PropertyValueMap::const_iterator it = d_ptr->m_values.constFind(property);
    if (it == d_ptr->m_values.constEnd())
        return QString();

    const QtFlagPropertyManagerPrivate::Data &data = it.value();

    QString str;
    int level = 0;
    const QChar bar = QLatin1Char('|');
    const QStringList::const_iterator fncend = data.flagNames.constEnd();
    for (QStringList::const_iterator it =  data.flagNames.constBegin(); it != fncend; ++it) {
        if (data.val & (1 << level)) {
            if (!str.isEmpty())
                str += bar;
            str += *it;
        }

        level++;
    }
    return str;
}

/*!
    \fn void QtFlagPropertyManager::setValue(QtProperty *property, int value)

    Sets the value of the given \a property to \a value. Nested
    properties are updated automatically.

    The specified \a value must be less than the binary combination of
    the property's flagNames() list size (i.e. less than 2\sup n,
    where \c n is the size of the list) and larger than (or equal to)
    0.

    \sa value(), valueChanged()
*/
void QtFlagPropertyManager::setValue(QtProperty *property, int val)
{
    const QtFlagPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    QtFlagPropertyManagerPrivate::Data data = it.value();

    if (data.val == val)
        return;

    if (val > (1 << data.flagNames.count()) - 1)
        return;

    if (val < 0)
        return;

    data.val = val;

    it.value() = data;

    QListIterator<QtProperty *> itProp(d_ptr->m_propertyToFlags[property]);
    int level = 0;
    while (itProp.hasNext()) {
        QtProperty *prop = itProp.next();
        if (prop)
            d_ptr->m_boolPropertyManager->setValue(prop, val & (1 << level));
        level++;
    }

    emit propertyChanged(property);
    emit valueChanged(property, data.val);
	emit QtAbstractPropertyManager::valueChanged(property, QVariant(data.val));
}


void QtFlagPropertyManager::setValue(QtProperty *property, const QVariant &val)
{
	Q_ASSERT(val.canConvert<int>());
	auto s_val = qvariant_cast<int>(val);
	setValue(property, s_val);
}


void QtFlagPropertyManager::setAttribute(QtProperty *property, const QString &attribute, const QVariant &value)
{
	if( attribute == "flagNames" ) {
		setFlagNames(property, qvariant_cast<QStringList>(value));
	} else {
		qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	}
}


/*!
    Sets the given \a property's list of flag names to \a flagNames. The
    property's current value is reset to 0 indicating the first item
    of the list.

    \sa flagNames(), flagNamesChanged()
*/
void QtFlagPropertyManager::setFlagNames(QtProperty *property, const QStringList &flagNames)
{
    const QtFlagPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    QtFlagPropertyManagerPrivate::Data data = it.value();

    if (data.flagNames == flagNames)
        return;

    data.flagNames = flagNames;
    data.val = 0;

    it.value() = data;

    QListIterator<QtProperty *> itProp(d_ptr->m_propertyToFlags[property]);
    while (itProp.hasNext()) {
        QtProperty *prop = itProp.next();
        if (prop) {
            delete prop;
            d_ptr->m_flagToProperty.remove(prop);
        }
    }
    d_ptr->m_propertyToFlags[property].clear();

    QStringListIterator itFlag(flagNames);
    while (itFlag.hasNext()) {
        const QString flagName = itFlag.next();
        QtProperty *prop = d_ptr->m_boolPropertyManager->addProperty();
        prop->setPropertyName(flagName);
        property->addSubProperty(prop);
        d_ptr->m_propertyToFlags[property].append(prop);
        d_ptr->m_flagToProperty[prop] = property;
    }

    emit flagNamesChanged(property, data.flagNames);
	emit attributeChanged(property, "flagNames", data.flagNames);

    emit propertyChanged(property);
    emit valueChanged(property, data.val);
	emit QtAbstractPropertyManager::valueChanged(property, data.val);
}

/*!
    \reimp
*/
void QtFlagPropertyManager::initializeProperty(QtProperty *property)
{
    d_ptr->m_values[property] = QtFlagPropertyManagerPrivate::Data();

    d_ptr->m_propertyToFlags[property] = QList<QtProperty *>();
}

/*!
    \reimp
*/
void QtFlagPropertyManager::uninitializeProperty(QtProperty *property)
{
    QListIterator<QtProperty *> itProp(d_ptr->m_propertyToFlags[property]);
    while (itProp.hasNext()) {
        QtProperty *prop = itProp.next();
        if (prop) {
            delete prop;
            d_ptr->m_flagToProperty.remove(prop);
        }
    }
    d_ptr->m_propertyToFlags.remove(property);

    d_ptr->m_values.remove(property);
}

// QtSizePolicyPropertyManager

QtSizePolicyPropertyManagerPrivate::QtSizePolicyPropertyManagerPrivate()
{
}

void QtSizePolicyPropertyManagerPrivate::slotIntChanged(QtProperty *property, int value)
{
    if (QtProperty *prop = m_hStretchToProperty.value(property, 0)) {
        QSizePolicy sp = m_values[prop];
        sp.setHorizontalStretch(value);
        q_ptr->setValue(prop, sp);
    } else if (QtProperty *prop = m_vStretchToProperty.value(property, 0)) {
        QSizePolicy sp = m_values[prop];
        sp.setVerticalStretch(value);
        q_ptr->setValue(prop, sp);
    }
}

void QtSizePolicyPropertyManagerPrivate::slotEnumChanged(QtProperty *property, int value)
{
    if (QtProperty *prop = m_hPolicyToProperty.value(property, 0)) {
        QSizePolicy sp = m_values[prop];
        sp.setHorizontalPolicy(metaEnumProvider()->indexToSizePolicy(value));
        q_ptr->setValue(prop, sp);
    } else if (QtProperty *prop = m_vPolicyToProperty.value(property, 0)) {
        QSizePolicy sp = m_values[prop];
        sp.setVerticalPolicy(metaEnumProvider()->indexToSizePolicy(value));
        q_ptr->setValue(prop, sp);
    }
}

void QtSizePolicyPropertyManagerPrivate::slotPropertyDestroyed(QtProperty *property)
{
    if (QtProperty *pointProp = m_hStretchToProperty.value(property, 0)) {
        m_propertyToHStretch[pointProp] = 0;
        m_hStretchToProperty.remove(property);
    } else if (QtProperty *pointProp = m_vStretchToProperty.value(property, 0)) {
        m_propertyToVStretch[pointProp] = 0;
        m_vStretchToProperty.remove(property);
    } else if (QtProperty *pointProp = m_hPolicyToProperty.value(property, 0)) {
        m_propertyToHPolicy[pointProp] = 0;
        m_hPolicyToProperty.remove(property);
    } else if (QtProperty *pointProp = m_vPolicyToProperty.value(property, 0)) {
        m_propertyToVPolicy[pointProp] = 0;
        m_vPolicyToProperty.remove(property);
    }
}

/*!
    \class QtSizePolicyPropertyManager

    \brief The QtSizePolicyPropertyManager provides and manages QSizePolicy properties.

    A size policy property has nested \e horizontalPolicy, \e
    verticalPolicy, \e horizontalStretch and \e verticalStretch
    subproperties. The top-level property's value can be retrieved
    using the value() function, and set using the setValue() slot.

    The subproperties are created by QtIntPropertyManager and QtEnumPropertyManager
    objects. These managers can be retrieved using the subIntPropertyManager()
    and subEnumPropertyManager() functions respectively. In order to provide
    editing widgets for the subproperties in a property browser widget,
    these managers must be associated with editor factories.

    In addition, QtSizePolicyPropertyManager provides the valueChanged()
    signal which is emitted whenever a property created by this
    manager changes.

    \sa QtAbstractPropertyManager, QtIntPropertyManager, QtEnumPropertyManager
*/

/*!
    \fn void QtSizePolicyPropertyManager::valueChanged(QtProperty *property, const QSizePolicy &value)

    This signal is emitted whenever a property created by this manager
    changes its value, passing a pointer to the \a property and the
    new \a value as parameters.

    \sa setValue()
*/

/*!
    Creates a manager with the given \a parent.
*/
QtSizePolicyPropertyManager::QtSizePolicyPropertyManager(QObject *parent)
    : QtAbstractPropertyManager(parent)
{
    d_ptr = new QtSizePolicyPropertyManagerPrivate;
    d_ptr->q_ptr = this;

    d_ptr->m_intPropertyManager = new QtIntPropertyManager(this);
    connect(d_ptr->m_intPropertyManager, SIGNAL(valueChanged(QtProperty *, int)),
                this, SLOT(slotIntChanged(QtProperty *, int)));
    d_ptr->m_enumPropertyManager = new QtEnumPropertyManager(this);
    connect(d_ptr->m_enumPropertyManager, SIGNAL(valueChanged(QtProperty *, int)),
                this, SLOT(slotEnumChanged(QtProperty *, int)));

    connect(d_ptr->m_intPropertyManager, SIGNAL(propertyDestroyed(QtProperty *)),
                this, SLOT(slotPropertyDestroyed(QtProperty *)));
    connect(d_ptr->m_enumPropertyManager, SIGNAL(propertyDestroyed(QtProperty *)),
                this, SLOT(slotPropertyDestroyed(QtProperty *)));
}

/*!
    Destroys this manager, and all the properties it has created.
*/
QtSizePolicyPropertyManager::~QtSizePolicyPropertyManager()
{
    clear();
    delete d_ptr;
}

/*!
    Returns the manager that creates the nested \e horizontalStretch
    and \e verticalStretch subproperties.

    In order to provide editing widgets for the mentioned subproperties
    in a property browser widget, this manager must be associated with
    an editor factory.

    \sa QtAbstractPropertyBrowser::setFactoryForManager()
*/
QtIntPropertyManager *QtSizePolicyPropertyManager::subIntPropertyManager() const
{
    return d_ptr->m_intPropertyManager;
}

/*!
    Returns the manager that creates the nested \e horizontalPolicy
    and \e verticalPolicy subproperties.

    In order to provide editing widgets for the mentioned subproperties
    in a property browser widget, this manager must be associated with
    an editor factory.

    \sa QtAbstractPropertyBrowser::setFactoryForManager()
*/
QtEnumPropertyManager *QtSizePolicyPropertyManager::subEnumPropertyManager() const
{
    return d_ptr->m_enumPropertyManager;
}

/*!
    Returns the given \a property's value.

    If the given property is not managed by this manager, this
    function returns the default size policy.

    \sa setValue()
*/
QSizePolicy QtSizePolicyPropertyManager::value(const QtProperty *property) const
{
    return d_ptr->m_values.value(property, QSizePolicy());
}


QVariant QtSizePolicyPropertyManager::attributeValue(const QtProperty *property, const QString &attribute) const
{
	Q_UNUSED(property);
	qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	return QVariant();
}


int QtSizePolicyPropertyManager::propertyTypeId() const
{
	return QVariant::SizePolicy;
}


QList<QtAbstractPropertyManager*> QtSizePolicyPropertyManager::subPropertyManagers() const
{
	return{ subIntPropertyManager(), subEnumPropertyManager() };
}


QVariant QtSizePolicyPropertyManager::variantValue(const QtProperty *property) const
{
	return QVariant::fromValue(value(property));
}


/*!
    \reimp
*/
QString QtSizePolicyPropertyManager::valueText(const QtProperty *property) const
{
    const QtSizePolicyPropertyManagerPrivate::PropertyValueMap::const_iterator it = d_ptr->m_values.constFind(property);
    if (it == d_ptr->m_values.constEnd())
        return QString();

    const QSizePolicy sp = it.value();
    const QtMetaEnumProvider *mep = metaEnumProvider();
    const int hIndex = mep->sizePolicyToIndex(sp.horizontalPolicy());
    const int vIndex = mep->sizePolicyToIndex(sp.verticalPolicy());
    //! Unknown size policy on reading invalid uic3 files
    const QString hPolicy = hIndex != -1 ? mep->policyEnumNames().at(hIndex) : tr("<Invalid>");
    const QString vPolicy = vIndex != -1 ? mep->policyEnumNames().at(vIndex) : tr("<Invalid>");
    const QString str = tr("[%1, %2, %3, %4]").arg(hPolicy, vPolicy).arg(sp.horizontalStretch()).arg(sp.verticalStretch());
    return str;
}

/*!
    \fn void QtSizePolicyPropertyManager::setValue(QtProperty *property, const QSizePolicy &value)

    Sets the value of the given \a property to \a value. Nested
    properties are updated automatically.

    \sa value(), valueChanged()
*/
void QtSizePolicyPropertyManager::setValue(QtProperty *property, const QSizePolicy &val)
{
    const QtSizePolicyPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    if (it.value() == val)
        return;

    it.value() = val;

    d_ptr->m_enumPropertyManager->setValue(d_ptr->m_propertyToHPolicy[property],
                metaEnumProvider()->sizePolicyToIndex(val.horizontalPolicy()));
    d_ptr->m_enumPropertyManager->setValue(d_ptr->m_propertyToVPolicy[property],
                metaEnumProvider()->sizePolicyToIndex(val.verticalPolicy()));
    d_ptr->m_intPropertyManager->setValue(d_ptr->m_propertyToHStretch[property],
                val.horizontalStretch());
    d_ptr->m_intPropertyManager->setValue(d_ptr->m_propertyToVStretch[property],
                val.verticalStretch());

    emit propertyChanged(property);
    emit valueChanged(property, val);
	emit QtAbstractPropertyManager::valueChanged(property, QVariant(val));
}


void QtSizePolicyPropertyManager::setValue(QtProperty *property, const QVariant &val)
{
	Q_ASSERT(val.canConvert<QSizePolicy>());
	auto s_val = qvariant_cast<QSizePolicy>(val);
	setValue(property, s_val);
}


void QtSizePolicyPropertyManager::setAttribute(QtProperty *property, const QString &attribute, const QVariant &value)
{
	Q_UNUSED(property); Q_UNUSED(value);
	qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
}


/*!
    \reimp
*/
void QtSizePolicyPropertyManager::initializeProperty(QtProperty *property)
{
    QSizePolicy val;
    d_ptr->m_values[property] = val;

    QtProperty *hPolicyProp = d_ptr->m_enumPropertyManager->addProperty();
    hPolicyProp->setPropertyName(tr("Horizontal Policy"));
    d_ptr->m_enumPropertyManager->setEnumNames(hPolicyProp, metaEnumProvider()->policyEnumNames());
    d_ptr->m_enumPropertyManager->setValue(hPolicyProp,
                metaEnumProvider()->sizePolicyToIndex(val.horizontalPolicy()));
    d_ptr->m_propertyToHPolicy[property] = hPolicyProp;
    d_ptr->m_hPolicyToProperty[hPolicyProp] = property;
    property->addSubProperty(hPolicyProp);

    QtProperty *vPolicyProp = d_ptr->m_enumPropertyManager->addProperty();
    vPolicyProp->setPropertyName(tr("Vertical Policy"));
    d_ptr->m_enumPropertyManager->setEnumNames(vPolicyProp, metaEnumProvider()->policyEnumNames());
    d_ptr->m_enumPropertyManager->setValue(vPolicyProp,
                metaEnumProvider()->sizePolicyToIndex(val.verticalPolicy()));
    d_ptr->m_propertyToVPolicy[property] = vPolicyProp;
    d_ptr->m_vPolicyToProperty[vPolicyProp] = property;
    property->addSubProperty(vPolicyProp);

    QtProperty *hStretchProp = d_ptr->m_intPropertyManager->addProperty();
    hStretchProp->setPropertyName(tr("Horizontal Stretch"));
    d_ptr->m_intPropertyManager->setValue(hStretchProp, val.horizontalStretch());
    d_ptr->m_intPropertyManager->setRange(hStretchProp, 0, 0xff);
    d_ptr->m_propertyToHStretch[property] = hStretchProp;
    d_ptr->m_hStretchToProperty[hStretchProp] = property;
    property->addSubProperty(hStretchProp);

    QtProperty *vStretchProp = d_ptr->m_intPropertyManager->addProperty();
    vStretchProp->setPropertyName(tr("Vertical Stretch"));
    d_ptr->m_intPropertyManager->setValue(vStretchProp, val.verticalStretch());
    d_ptr->m_intPropertyManager->setRange(vStretchProp, 0, 0xff);
    d_ptr->m_propertyToVStretch[property] = vStretchProp;
    d_ptr->m_vStretchToProperty[vStretchProp] = property;
    property->addSubProperty(vStretchProp);

}

/*!
    \reimp
*/
void QtSizePolicyPropertyManager::uninitializeProperty(QtProperty *property)
{
    QtProperty *hPolicyProp = d_ptr->m_propertyToHPolicy[property];
    if (hPolicyProp) {
        d_ptr->m_hPolicyToProperty.remove(hPolicyProp);
        delete hPolicyProp;
    }
    d_ptr->m_propertyToHPolicy.remove(property);

    QtProperty *vPolicyProp = d_ptr->m_propertyToVPolicy[property];
    if (vPolicyProp) {
        d_ptr->m_vPolicyToProperty.remove(vPolicyProp);
        delete vPolicyProp;
    }
    d_ptr->m_propertyToVPolicy.remove(property);

    QtProperty *hStretchProp = d_ptr->m_propertyToHStretch[property];
    if (hStretchProp) {
        d_ptr->m_hStretchToProperty.remove(hStretchProp);
        delete hStretchProp;
    }
    d_ptr->m_propertyToHStretch.remove(property);

    QtProperty *vStretchProp = d_ptr->m_propertyToVStretch[property];
    if (vStretchProp) {
        d_ptr->m_vStretchToProperty.remove(vStretchProp);
        delete vStretchProp;
    }
    d_ptr->m_propertyToVStretch.remove(property);

    d_ptr->m_values.remove(property);
}

// QtFontPropertyManager:
// QtFontPropertyManagerPrivate has a mechanism for reacting
// to QApplication::fontDatabaseChanged() [4.5], which is emitted
// when someone loads an application font. The signals are compressed
// using a timer with interval 0, which then causes the family
// enumeration manager to re-set its strings and index values
// for each property.

Q_GLOBAL_STATIC(QFontDatabase, fontDatabase)

QtFontPropertyManagerPrivate::QtFontPropertyManagerPrivate() :
    m_settingValue(false),
    m_fontDatabaseChangeTimer(0)
{
}

void QtFontPropertyManagerPrivate::slotIntChanged(QtProperty *property, int value)
{
    if (m_settingValue)
        return;
    if (QtProperty *prop = m_pointSizeToProperty.value(property, 0)) {
        QFont f = m_values[prop];
        f.setPointSize(value);
        q_ptr->setValue(prop, f);
    }
}

void QtFontPropertyManagerPrivate::slotEnumChanged(QtProperty *property, int value)
{
    if (m_settingValue)
        return;
    if (QtProperty *prop = m_familyToProperty.value(property, 0)) {
        QFont f = m_values[prop];
        f.setFamily(m_familyNames.at(value));
        q_ptr->setValue(prop, f);
    }
}

void QtFontPropertyManagerPrivate::slotBoolChanged(QtProperty *property, bool value)
{
    if (m_settingValue)
        return;
    if (QtProperty *prop = m_boldToProperty.value(property, 0)) {
        QFont f = m_values[prop];
        f.setBold(value);
        q_ptr->setValue(prop, f);
    } else if (QtProperty *prop = m_italicToProperty.value(property, 0)) {
        QFont f = m_values[prop];
        f.setItalic(value);
        q_ptr->setValue(prop, f);
    } else if (QtProperty *prop = m_underlineToProperty.value(property, 0)) {
        QFont f = m_values[prop];
        f.setUnderline(value);
        q_ptr->setValue(prop, f);
    } else if (QtProperty *prop = m_strikeOutToProperty.value(property, 0)) {
        QFont f = m_values[prop];
        f.setStrikeOut(value);
        q_ptr->setValue(prop, f);
    } else if (QtProperty *prop = m_kerningToProperty.value(property, 0)) {
        QFont f = m_values[prop];
        f.setKerning(value);
        q_ptr->setValue(prop, f);
    }
}

void QtFontPropertyManagerPrivate::slotPropertyDestroyed(QtProperty *property)
{
    if (QtProperty *pointProp = m_pointSizeToProperty.value(property, 0)) {
        m_propertyToPointSize[pointProp] = 0;
        m_pointSizeToProperty.remove(property);
    } else if (QtProperty *pointProp = m_familyToProperty.value(property, 0)) {
        m_propertyToFamily[pointProp] = 0;
        m_familyToProperty.remove(property);
    } else if (QtProperty *pointProp = m_boldToProperty.value(property, 0)) {
        m_propertyToBold[pointProp] = 0;
        m_boldToProperty.remove(property);
    } else if (QtProperty *pointProp = m_italicToProperty.value(property, 0)) {
        m_propertyToItalic[pointProp] = 0;
        m_italicToProperty.remove(property);
    } else if (QtProperty *pointProp = m_underlineToProperty.value(property, 0)) {
        m_propertyToUnderline[pointProp] = 0;
        m_underlineToProperty.remove(property);
    } else if (QtProperty *pointProp = m_strikeOutToProperty.value(property, 0)) {
        m_propertyToStrikeOut[pointProp] = 0;
        m_strikeOutToProperty.remove(property);
    } else if (QtProperty *pointProp = m_kerningToProperty.value(property, 0)) {
        m_propertyToKerning[pointProp] = 0;
        m_kerningToProperty.remove(property);
    }
}

void  QtFontPropertyManagerPrivate::slotFontDatabaseChanged()
{
    if (!m_fontDatabaseChangeTimer) {
        m_fontDatabaseChangeTimer = new QTimer(q_ptr);
        m_fontDatabaseChangeTimer->setInterval(0);
        m_fontDatabaseChangeTimer->setSingleShot(true);
        QObject::connect(m_fontDatabaseChangeTimer, SIGNAL(timeout()), q_ptr, SLOT(slotFontDatabaseDelayedChange()));
    }
    if (!m_fontDatabaseChangeTimer->isActive())
        m_fontDatabaseChangeTimer->start();
}

void QtFontPropertyManagerPrivate::slotFontDatabaseDelayedChange()
{
    typedef QMap<const QtProperty *, QtProperty *> PropertyPropertyMap;
    // rescan available font names
    const QStringList oldFamilies = m_familyNames;
    m_familyNames = fontDatabase()->families();

    // Adapt all existing properties
    if (!m_propertyToFamily.empty()) {
        PropertyPropertyMap::const_iterator cend = m_propertyToFamily.constEnd();
        for (PropertyPropertyMap::const_iterator it = m_propertyToFamily.constBegin(); it != cend; ++it) {
            QtProperty *familyProp = it.value();
            const int oldIdx = m_enumPropertyManager->value(familyProp);
            int newIdx = m_familyNames.indexOf(oldFamilies.at(oldIdx));
            if (newIdx < 0)
                newIdx = 0;
            m_enumPropertyManager->setEnumNames(familyProp, m_familyNames);
            m_enumPropertyManager->setValue(familyProp, newIdx);
        }
    }
}

/*!
    \class QtFontPropertyManager

    \brief The QtFontPropertyManager provides and manages QFont properties.

    A font property has nested \e family, \e pointSize, \e bold, \e
    italic, \e underline, \e strikeOut and \e kerning subproperties. The top-level
    property's value can be retrieved using the value() function, and
    set using the setValue() slot.

    The subproperties are created by QtIntPropertyManager, QtEnumPropertyManager and
    QtBoolPropertyManager objects. These managers can be retrieved using the
    corresponding subIntPropertyManager(), subEnumPropertyManager() and
    subBoolPropertyManager() functions. In order to provide editing widgets
    for the subproperties in a property browser widget, these managers
    must be associated with editor factories.

    In addition, QtFontPropertyManager provides the valueChanged() signal
    which is emitted whenever a property created by this manager
    changes.

    \sa QtAbstractPropertyManager, QtEnumPropertyManager, QtIntPropertyManager, QtBoolPropertyManager
*/

/*!
    \fn void QtFontPropertyManager::valueChanged(QtProperty *property, const QFont &value)

    This signal is emitted whenever a property created by this manager
    changes its value, passing a pointer to the \a property and the
    new \a value as parameters.

    \sa setValue()
*/

/*!
    Creates a manager with the given \a parent.
*/
QtFontPropertyManager::QtFontPropertyManager(QObject *parent)
    : QtAbstractPropertyManager(parent)
{
    d_ptr = new QtFontPropertyManagerPrivate;
    d_ptr->q_ptr = this;
#if QT_VERSION >= 0x040500
    QObject::connect(qApp, SIGNAL(fontDatabaseChanged()), this, SLOT(slotFontDatabaseChanged()));
#endif

    d_ptr->m_intPropertyManager = new QtIntPropertyManager(this);
    connect(d_ptr->m_intPropertyManager, SIGNAL(valueChanged(QtProperty *, int)),
                this, SLOT(slotIntChanged(QtProperty *, int)));
    d_ptr->m_enumPropertyManager = new QtEnumPropertyManager(this);
    connect(d_ptr->m_enumPropertyManager, SIGNAL(valueChanged(QtProperty *, int)),
                this, SLOT(slotEnumChanged(QtProperty *, int)));
    d_ptr->m_boolPropertyManager = new QtBoolPropertyManager(this);
    connect(d_ptr->m_boolPropertyManager, SIGNAL(valueChanged(QtProperty *, bool)),
                this, SLOT(slotBoolChanged(QtProperty *, bool)));

    connect(d_ptr->m_intPropertyManager, SIGNAL(propertyDestroyed(QtProperty *)),
                this, SLOT(slotPropertyDestroyed(QtProperty *)));
    connect(d_ptr->m_enumPropertyManager, SIGNAL(propertyDestroyed(QtProperty *)),
                this, SLOT(slotPropertyDestroyed(QtProperty *)));
    connect(d_ptr->m_boolPropertyManager, SIGNAL(propertyDestroyed(QtProperty *)),
                this, SLOT(slotPropertyDestroyed(QtProperty *)));
}

/*!
    Destroys this manager, and all the properties it has created.
*/
QtFontPropertyManager::~QtFontPropertyManager()
{
    clear();
    delete d_ptr;
}

/*!
    Returns the manager that creates the \e pointSize subproperty.

    In order to provide editing widgets for the \e pointSize property
    in a property browser widget, this manager must be associated
    with an editor factory.

    \sa QtAbstractPropertyBrowser::setFactoryForManager()
*/
QtIntPropertyManager *QtFontPropertyManager::subIntPropertyManager() const
{
    return d_ptr->m_intPropertyManager;
}

/*!
    Returns the manager that create the \e family subproperty.

    In order to provide editing widgets for the \e family property
    in a property browser widget, this manager must be associated
    with an editor factory.

    \sa QtAbstractPropertyBrowser::setFactoryForManager()
*/
QtEnumPropertyManager *QtFontPropertyManager::subEnumPropertyManager() const
{
    return d_ptr->m_enumPropertyManager;
}

/*!
    Returns the manager that creates the  \e bold, \e italic, \e underline,
    \e strikeOut and \e kerning subproperties.

    In order to provide editing widgets for the mentioned properties
    in a property browser widget, this manager must be associated with
    an editor factory.

    \sa QtAbstractPropertyBrowser::setFactoryForManager()
*/
QtBoolPropertyManager *QtFontPropertyManager::subBoolPropertyManager() const
{
    return d_ptr->m_boolPropertyManager;
}

/*!
    Returns the given \a property's value.

    If the given property is not managed by this manager, this
    function returns a font object that uses the application's default
    font.

    \sa setValue()
*/
QFont QtFontPropertyManager::value(const QtProperty *property) const
{
    return d_ptr->m_values.value(property, QFont());
}


QVariant QtFontPropertyManager::attributeValue(const QtProperty *property, const QString &attribute) const
{
	Q_UNUSED(property);
	qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	return QVariant();
}


int QtFontPropertyManager::propertyTypeId() const
{
	return QVariant::Font;
}


QList<QtAbstractPropertyManager*> QtFontPropertyManager::subPropertyManagers() const
{
	return{ subIntPropertyManager(), subEnumPropertyManager(), subBoolPropertyManager() };
}


QVariant QtFontPropertyManager::variantValue(const QtProperty *property) const
{
	return QVariant::fromValue(value(property));
}


/*!
    \reimp
*/
QString QtFontPropertyManager::valueText(const QtProperty *property) const
{
    const QtFontPropertyManagerPrivate::PropertyValueMap::const_iterator it = d_ptr->m_values.constFind(property);
    if (it == d_ptr->m_values.constEnd())
        return QString();

    return QtPropertyBrowserUtils::fontValueText(it.value());
}

/*!
    \reimp
*/
QIcon QtFontPropertyManager::valueIcon(const QtProperty *property) const
{
    const QtFontPropertyManagerPrivate::PropertyValueMap::const_iterator it = d_ptr->m_values.constFind(property);
    if (it == d_ptr->m_values.constEnd())
        return QIcon();

    return QtPropertyBrowserUtils::fontValueIcon(it.value());
}

/*!
    \fn void QtFontPropertyManager::setValue(QtProperty *property, const QFont &value)

    Sets the value of the given \a property to \a value. Nested
    properties are updated automatically.

    \sa value(), valueChanged()
*/
void QtFontPropertyManager::setValue(QtProperty *property, const QFont &val)
{
    const QtFontPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    const QFont oldVal = it.value();
    if (oldVal == val && oldVal.resolve() == val.resolve())
        return;

    it.value() = val;

    int idx = d_ptr->m_familyNames.indexOf(val.family());
    if (idx == -1)
        idx = 0;
    bool settingValue = d_ptr->m_settingValue;
    d_ptr->m_settingValue = true;
    d_ptr->m_enumPropertyManager->setValue(d_ptr->m_propertyToFamily[property], idx);
    d_ptr->m_intPropertyManager->setValue(d_ptr->m_propertyToPointSize[property], val.pointSize());
    d_ptr->m_boolPropertyManager->setValue(d_ptr->m_propertyToBold[property], val.bold());
    d_ptr->m_boolPropertyManager->setValue(d_ptr->m_propertyToItalic[property], val.italic());
    d_ptr->m_boolPropertyManager->setValue(d_ptr->m_propertyToUnderline[property], val.underline());
    d_ptr->m_boolPropertyManager->setValue(d_ptr->m_propertyToStrikeOut[property], val.strikeOut());
    d_ptr->m_boolPropertyManager->setValue(d_ptr->m_propertyToKerning[property], val.kerning());
    d_ptr->m_settingValue = settingValue;

    emit propertyChanged(property);
    emit valueChanged(property, val);
	emit QtAbstractPropertyManager::valueChanged(property, QVariant(val));
}


void QtFontPropertyManager::setValue(QtProperty *property, const QVariant &val)
{
	Q_ASSERT(val.canConvert<QFont>());
	auto s_val = qvariant_cast<QFont>(val);
	setValue(property, s_val);
}


void QtFontPropertyManager::setAttribute(QtProperty *property, const QString &attribute, const QVariant &value)
{
	Q_UNUSED(property); Q_UNUSED(value);
	qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
}


/*!
    \reimp
*/
void QtFontPropertyManager::initializeProperty(QtProperty *property)
{
    QFont val;
    d_ptr->m_values[property] = val;

    QtProperty *familyProp = d_ptr->m_enumPropertyManager->addProperty();
    familyProp->setPropertyName(tr("Family"));
    if (d_ptr->m_familyNames.empty())
        d_ptr->m_familyNames = fontDatabase()->families();
    d_ptr->m_enumPropertyManager->setEnumNames(familyProp, d_ptr->m_familyNames);
    int idx = d_ptr->m_familyNames.indexOf(val.family());
    if (idx == -1)
        idx = 0;
    d_ptr->m_enumPropertyManager->setValue(familyProp, idx);
    d_ptr->m_propertyToFamily[property] = familyProp;
    d_ptr->m_familyToProperty[familyProp] = property;
    property->addSubProperty(familyProp);

    QtProperty *pointSizeProp = d_ptr->m_intPropertyManager->addProperty();
    pointSizeProp->setPropertyName(tr("Point Size"));
    d_ptr->m_intPropertyManager->setValue(pointSizeProp, val.pointSize());
    d_ptr->m_intPropertyManager->setMinimum(pointSizeProp, 1);
    d_ptr->m_propertyToPointSize[property] = pointSizeProp;
    d_ptr->m_pointSizeToProperty[pointSizeProp] = property;
    property->addSubProperty(pointSizeProp);

    QtProperty *boldProp = d_ptr->m_boolPropertyManager->addProperty();
    boldProp->setPropertyName(tr("Bold"));
    d_ptr->m_boolPropertyManager->setValue(boldProp, val.bold());
    d_ptr->m_propertyToBold[property] = boldProp;
    d_ptr->m_boldToProperty[boldProp] = property;
    property->addSubProperty(boldProp);

    QtProperty *italicProp = d_ptr->m_boolPropertyManager->addProperty();
    italicProp->setPropertyName(tr("Italic"));
    d_ptr->m_boolPropertyManager->setValue(italicProp, val.italic());
    d_ptr->m_propertyToItalic[property] = italicProp;
    d_ptr->m_italicToProperty[italicProp] = property;
    property->addSubProperty(italicProp);

    QtProperty *underlineProp = d_ptr->m_boolPropertyManager->addProperty();
    underlineProp->setPropertyName(tr("Underline"));
    d_ptr->m_boolPropertyManager->setValue(underlineProp, val.underline());
    d_ptr->m_propertyToUnderline[property] = underlineProp;
    d_ptr->m_underlineToProperty[underlineProp] = property;
    property->addSubProperty(underlineProp);

    QtProperty *strikeOutProp = d_ptr->m_boolPropertyManager->addProperty();
    strikeOutProp->setPropertyName(tr("Strikeout"));
    d_ptr->m_boolPropertyManager->setValue(strikeOutProp, val.strikeOut());
    d_ptr->m_propertyToStrikeOut[property] = strikeOutProp;
    d_ptr->m_strikeOutToProperty[strikeOutProp] = property;
    property->addSubProperty(strikeOutProp);

    QtProperty *kerningProp = d_ptr->m_boolPropertyManager->addProperty();
    kerningProp->setPropertyName(tr("Kerning"));
    d_ptr->m_boolPropertyManager->setValue(kerningProp, val.kerning());
    d_ptr->m_propertyToKerning[property] = kerningProp;
    d_ptr->m_kerningToProperty[kerningProp] = property;
    property->addSubProperty(kerningProp);
}

/*!
    \reimp
*/
void QtFontPropertyManager::uninitializeProperty(QtProperty *property)
{
    QtProperty *familyProp = d_ptr->m_propertyToFamily[property];
    if (familyProp) {
        d_ptr->m_familyToProperty.remove(familyProp);
        delete familyProp;
    }
    d_ptr->m_propertyToFamily.remove(property);

    QtProperty *pointSizeProp = d_ptr->m_propertyToPointSize[property];
    if (pointSizeProp) {
        d_ptr->m_pointSizeToProperty.remove(pointSizeProp);
        delete pointSizeProp;
    }
    d_ptr->m_propertyToPointSize.remove(property);

    QtProperty *boldProp = d_ptr->m_propertyToBold[property];
    if (boldProp) {
        d_ptr->m_boldToProperty.remove(boldProp);
        delete boldProp;
    }
    d_ptr->m_propertyToBold.remove(property);

    QtProperty *italicProp = d_ptr->m_propertyToItalic[property];
    if (italicProp) {
        d_ptr->m_italicToProperty.remove(italicProp);
        delete italicProp;
    }
    d_ptr->m_propertyToItalic.remove(property);

    QtProperty *underlineProp = d_ptr->m_propertyToUnderline[property];
    if (underlineProp) {
        d_ptr->m_underlineToProperty.remove(underlineProp);
        delete underlineProp;
    }
    d_ptr->m_propertyToUnderline.remove(property);

    QtProperty *strikeOutProp = d_ptr->m_propertyToStrikeOut[property];
    if (strikeOutProp) {
        d_ptr->m_strikeOutToProperty.remove(strikeOutProp);
        delete strikeOutProp;
    }
    d_ptr->m_propertyToStrikeOut.remove(property);

    QtProperty *kerningProp = d_ptr->m_propertyToKerning[property];
    if (kerningProp) {
        d_ptr->m_kerningToProperty.remove(kerningProp);
        delete kerningProp;
    }
    d_ptr->m_propertyToKerning.remove(property);

    d_ptr->m_values.remove(property);
}

// QtColorPropertyManager

void QtColorPropertyManagerPrivate::slotIntChanged(QtProperty *property, int value)
{
    if (QtProperty *prop = m_rToProperty.value(property, 0)) {
        QColor c = m_values[prop];
        c.setRed(value);
        q_ptr->setValue(prop, c);
    } else if (QtProperty *prop = m_gToProperty.value(property, 0)) {
        QColor c = m_values[prop];
        c.setGreen(value);
        q_ptr->setValue(prop, c);
    } else if (QtProperty *prop = m_bToProperty.value(property, 0)) {
        QColor c = m_values[prop];
        c.setBlue(value);
        q_ptr->setValue(prop, c);
    } else if (QtProperty *prop = m_aToProperty.value(property, 0)) {
        QColor c = m_values[prop];
        c.setAlpha(value);
        q_ptr->setValue(prop, c);
    }
}

void QtColorPropertyManagerPrivate::slotPropertyDestroyed(QtProperty *property)
{
    if (QtProperty *pointProp = m_rToProperty.value(property, 0)) {
        m_propertyToR[pointProp] = 0;
        m_rToProperty.remove(property);
    } else if (QtProperty *pointProp = m_gToProperty.value(property, 0)) {
        m_propertyToG[pointProp] = 0;
        m_gToProperty.remove(property);
    } else if (QtProperty *pointProp = m_bToProperty.value(property, 0)) {
        m_propertyToB[pointProp] = 0;
        m_bToProperty.remove(property);
    } else if (QtProperty *pointProp = m_aToProperty.value(property, 0)) {
        m_propertyToA[pointProp] = 0;
        m_aToProperty.remove(property);
    }
}

/*!
    \class QtColorPropertyManager

    \brief The QtColorPropertyManager provides and manages QColor properties.

    A color property has nested \e red, \e green and \e blue
    subproperties. The top-level property's value can be retrieved
    using the value() function, and set using the setValue() slot.

    The subproperties are created by a QtIntPropertyManager object. This
    manager can be retrieved using the subIntPropertyManager() function.  In
    order to provide editing widgets for the subproperties in a
    property browser widget, this manager must be associated with an
    editor factory.

    In addition, QtColorPropertyManager provides the valueChanged() signal
    which is emitted whenever a property created by this manager
    changes.

    \sa QtAbstractPropertyManager, QtAbstractPropertyBrowser, QtIntPropertyManager
*/

/*!
    \fn void QtColorPropertyManager::valueChanged(QtProperty *property, const QColor &value)

    This signal is emitted whenever a property created by this manager
    changes its value, passing a pointer to the \a property and the new
    \a value as parameters.

    \sa setValue()
*/

/*!
    Creates a manager with the given \a parent.
*/
QtColorPropertyManager::QtColorPropertyManager(QObject *parent)
    : QtAbstractPropertyManager(parent)
{
    d_ptr = new QtColorPropertyManagerPrivate;
    d_ptr->q_ptr = this;

    d_ptr->m_intPropertyManager = new QtIntPropertyManager(this);
    connect(d_ptr->m_intPropertyManager, SIGNAL(valueChanged(QtProperty *, int)),
                this, SLOT(slotIntChanged(QtProperty *, int)));

    connect(d_ptr->m_intPropertyManager, SIGNAL(propertyDestroyed(QtProperty *)),
                this, SLOT(slotPropertyDestroyed(QtProperty *)));
}

/*!
    Destroys this manager, and all the properties it has created.
*/
QtColorPropertyManager::~QtColorPropertyManager()
{
    clear();
    delete d_ptr;
}

/*!
    Returns the manager that produces the nested \e red, \e green and
    \e blue subproperties.

    In order to provide editing widgets for the subproperties in a
    property browser widget, this manager must be associated with an
    editor factory.

    \sa QtAbstractPropertyBrowser::setFactoryForManager()
*/
QtIntPropertyManager *QtColorPropertyManager::subIntPropertyManager() const
{
    return d_ptr->m_intPropertyManager;
}

/*!
    Returns the given \a property's value.

    If the given \a property is not managed by \e this manager, this
    function returns an invalid color.

    \sa setValue()
*/
QColor QtColorPropertyManager::value(const QtProperty *property) const
{
    return d_ptr->m_values.value(property, QColor());
}


QVariant QtColorPropertyManager::attributeValue(const QtProperty *property, const QString &attribute) const
{
	if( attribute == "alphaHidden" ) {
		return QVariant(alphaHidden(property));
	} else {
		qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	}
	return QVariant();
}


int QtColorPropertyManager::propertyTypeId() const
{
	return QVariant::Color;
}


QList<QtAbstractPropertyManager*> QtColorPropertyManager::subPropertyManagers() const
{
	return{ subIntPropertyManager() };
}


bool QtColorPropertyManager::alphaHidden(const QtProperty *property) const
{
    auto        it = d_ptr->m_alphaHiddenMap.constFind(property);
	if( it == d_ptr->m_alphaHiddenMap.constEnd() ) {
		return false;
	}

	return (*it);
}


QVariant QtColorPropertyManager::variantValue(const QtProperty *property) const
{
	return QVariant::fromValue(value(property));
}


/*!
    \reimp
*/

QString QtColorPropertyManager::valueText(const QtProperty *property) const
{
    const QtColorPropertyManagerPrivate::PropertyValueMap::const_iterator it = d_ptr->m_values.constFind(property);
	if( it == d_ptr->m_values.constEnd() ) {
		return QString();
	}

	bool        alphaHidden = false;
    auto        it2 = d_ptr->m_alphaHiddenMap.constFind(property);
	if( it2 != d_ptr->m_alphaHiddenMap.constEnd() ) {
		alphaHidden = it2.value();
	}

	return QtPropertyBrowserUtils::colorValueText(it.value(), alphaHidden);
}

/*!
    \reimp
*/

QIcon QtColorPropertyManager::valueIcon(const QtProperty *property) const
{
    const QtColorPropertyManagerPrivate::PropertyValueMap::const_iterator it = d_ptr->m_values.constFind(property);
    if (it == d_ptr->m_values.constEnd())
        return QIcon();
    return QtPropertyBrowserUtils::brushValueIcon(QBrush(it.value()));
}

/*!
    \fn void QtColorPropertyManager::setValue(QtProperty *property, const QColor &value)

    Sets the value of the given \a property to \a value.  Nested
    properties are updated automatically.

    \sa value(), valueChanged()
*/
void QtColorPropertyManager::setValue(QtProperty *property, const QColor &val)
{
    const QtColorPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    if (it.value() == val)
        return;

    it.value() = val;

    d_ptr->m_intPropertyManager->setValue(d_ptr->m_propertyToR[property], val.red());
    d_ptr->m_intPropertyManager->setValue(d_ptr->m_propertyToG[property], val.green());
    d_ptr->m_intPropertyManager->setValue(d_ptr->m_propertyToB[property], val.blue());
    d_ptr->m_intPropertyManager->setValue(d_ptr->m_propertyToA[property], val.alpha());

    emit propertyChanged(property);
    emit valueChanged(property, val);
	emit QtAbstractPropertyManager::valueChanged(property, QVariant(val));
}


void QtColorPropertyManager::setValue(QtProperty *property, const QVariant &val)
{
	Q_ASSERT(val.canConvert<QColor>());
	auto s_val = qvariant_cast<QColor>(val);
	setValue(property, s_val);
}


void QtColorPropertyManager::setAttribute(QtProperty *property, const QString &attribute, const QVariant &value)
{
	if( attribute == "alphaHidden" ) {
		setAlphaHidden(property, qvariant_cast<bool>(value));
	} else {
		qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	}
}


void QtColorPropertyManager::setAlphaHidden(QtProperty *property, bool alphaHidden)
{
    auto it = d_ptr->m_alphaHiddenMap.find(property);
	if( it == d_ptr->m_alphaHiddenMap.end() ) {
		return;
	}

	if( it.value() == alphaHidden ) {
		return;
	}

	it.value() = alphaHidden;

	if( alphaHidden ) {
		property->removeSubProperty(d_ptr->m_propertyToA[property]);
	} else {
		property->addSubProperty(d_ptr->m_propertyToA[property]);
	}

	emit alphaHiddenChanged(property, alphaHidden);
	emit attributeChanged(property, "alphaHidden", alphaHidden);
	emit propertyChanged(property);
}


/*!
    \reimp
*/
void QtColorPropertyManager::initializeProperty(QtProperty *property)
{
    QColor val;
    d_ptr->m_values[property] = val;
	d_ptr->m_alphaHiddenMap[property] = false;

    QtProperty *rProp = d_ptr->m_intPropertyManager->addProperty();
    rProp->setPropertyName(tr("Red"));
    d_ptr->m_intPropertyManager->setValue(rProp, val.red());
    d_ptr->m_intPropertyManager->setRange(rProp, 0, 0xFF);
    d_ptr->m_propertyToR[property] = rProp;
    d_ptr->m_rToProperty[rProp] = property;
    property->addSubProperty(rProp);

    QtProperty *gProp = d_ptr->m_intPropertyManager->addProperty();
    gProp->setPropertyName(tr("Green"));
    d_ptr->m_intPropertyManager->setValue(gProp, val.green());
    d_ptr->m_intPropertyManager->setRange(gProp, 0, 0xFF);
    d_ptr->m_propertyToG[property] = gProp;
    d_ptr->m_gToProperty[gProp] = property;
    property->addSubProperty(gProp);

    QtProperty *bProp = d_ptr->m_intPropertyManager->addProperty();
    bProp->setPropertyName(tr("Blue"));
    d_ptr->m_intPropertyManager->setValue(bProp, val.blue());
    d_ptr->m_intPropertyManager->setRange(bProp, 0, 0xFF);
    d_ptr->m_propertyToB[property] = bProp;
    d_ptr->m_bToProperty[bProp] = property;
    property->addSubProperty(bProp);

    QtProperty *aProp = d_ptr->m_intPropertyManager->addProperty();
    aProp->setPropertyName(tr("Alpha"));
    d_ptr->m_intPropertyManager->setValue(aProp, val.alpha());
    d_ptr->m_intPropertyManager->setRange(aProp, 0, 0xFF);
    d_ptr->m_propertyToA[property] = aProp;
    d_ptr->m_aToProperty[aProp] = property;
    property->addSubProperty(aProp);
}

/*!
    \reimp
*/
void QtColorPropertyManager::uninitializeProperty(QtProperty *property)
{
    QtProperty *rProp = d_ptr->m_propertyToR[property];
    if (rProp) {
        d_ptr->m_rToProperty.remove(rProp);
        delete rProp;
    }
    d_ptr->m_propertyToR.remove(property);

    QtProperty *gProp = d_ptr->m_propertyToG[property];
    if (gProp) {
        d_ptr->m_gToProperty.remove(gProp);
        delete gProp;
    }
    d_ptr->m_propertyToG.remove(property);

    QtProperty *bProp = d_ptr->m_propertyToB[property];
    if (bProp) {
        d_ptr->m_bToProperty.remove(bProp);
        delete bProp;
    }
    d_ptr->m_propertyToB.remove(property);

    QtProperty *aProp = d_ptr->m_propertyToA[property];
    if (aProp) {
        d_ptr->m_aToProperty.remove(aProp);
        delete aProp;
    }
    d_ptr->m_propertyToA.remove(property);

    d_ptr->m_alphaHiddenMap.remove(property);
    d_ptr->m_values.remove(property);
}

// QtCursorPropertyManager

// Make sure icons are removed as soon as QApplication is destroyed, otherwise,
// handles are leaked on X11.
static void clearCursorDatabase();
namespace {
struct CursorDatabase : public QtCursorDatabase
{
    CursorDatabase()
    {
        qAddPostRoutine(clearCursorDatabase);
    }
};
}
Q_GLOBAL_STATIC(QtCursorDatabase, cursorDatabase)

static void clearCursorDatabase()
{
    cursorDatabase()->clear();
}

/*!
    \class QtCursorPropertyManager

    \brief The QtCursorPropertyManager provides and manages QCursor properties.

    A cursor property has a current value which can be
    retrieved using the value() function, and set using the setValue()
    slot. In addition, QtCursorPropertyManager provides the
    valueChanged() signal which is emitted whenever a property created
    by this manager changes.

    \sa QtAbstractPropertyManager
*/

/*!
    \fn void QtCursorPropertyManager::valueChanged(QtProperty *property, const QCursor &value)

    This signal is emitted whenever a property created by this manager
    changes its value, passing a pointer to the \a property and the new
    \a value as parameters.

    \sa setValue()
*/

/*!
    Creates a manager with the given \a parent.
*/
QtCursorPropertyManager::QtCursorPropertyManager(QObject *parent)
    : QtAbstractPropertyManager(parent)
{
    d_ptr = new QtCursorPropertyManagerPrivate;
    d_ptr->q_ptr = this;
}

/*!
    Destroys this manager, and all the properties it has created.
*/
QtCursorPropertyManager::~QtCursorPropertyManager()
{
    clear();
    delete d_ptr;
}

/*!
    Returns the given \a property's value.

    If the given \a property is not managed by this manager, this
    function returns a default QCursor object.

    \sa setValue()
*/
#ifndef QT_NO_CURSOR
QCursor QtCursorPropertyManager::value(const QtProperty *property) const
{
    return d_ptr->m_values.value(property, QCursor());
}


QVariant QtCursorPropertyManager::variantValue(const QtProperty *property) const
{
	return QVariant::fromValue(value(property));
}

#endif

/*!
    \reimp
*/
QString QtCursorPropertyManager::valueText(const QtProperty *property) const
{
   const QtCursorPropertyManagerPrivate::PropertyValueMap::const_iterator it = d_ptr->m_values.constFind(property);
    if (it == d_ptr->m_values.constEnd())
        return QString();

    return cursorDatabase()->cursorToShapeName(it.value());
}

/*!
    \reimp
*/
QIcon QtCursorPropertyManager::valueIcon(const QtProperty *property) const
{
    const QtCursorPropertyManagerPrivate::PropertyValueMap::const_iterator it = d_ptr->m_values.constFind(property);
    if (it == d_ptr->m_values.constEnd())
        return QIcon();

    return cursorDatabase()->cursorToShapeIcon(it.value());
}


QVariant QtCursorPropertyManager::attributeValue(const QtProperty *property, const QString &attribute) const
{
	Q_UNUSED(property);
	qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
	return QVariant();
}


int QtCursorPropertyManager::propertyTypeId() const
{
	return QVariant::Cursor;
}


/*!
    \fn void QtCursorPropertyManager::setValue(QtProperty *property, const QCursor &value)

    Sets the value of the given \a property to \a value.

    \sa value(), valueChanged()
*/
void QtCursorPropertyManager::setValue(QtProperty *property, const QCursor &value)
{
#ifndef QT_NO_CURSOR
    const QtCursorPropertyManagerPrivate::PropertyValueMap::iterator it = d_ptr->m_values.find(property);
    if (it == d_ptr->m_values.end())
        return;

    if (it.value().shape() == value.shape() && value.shape() != Qt::BitmapCursor)
        return;

    it.value() = value;

    emit propertyChanged(property);
    emit valueChanged(property, value);
	emit QtAbstractPropertyManager::valueChanged(property, QVariant(value));
#endif
}


void QtCursorPropertyManager::setValue(QtProperty *property, const QVariant &val)
{
#ifndef QT_NO_CURSOR
	Q_ASSERT(val.canConvert<QCursor>());
	auto s_val = qvariant_cast<QCursor>(val);
	setValue(property, s_val);
#endif
}


void QtCursorPropertyManager::setAttribute(QtProperty *property, const QString &attribute, const QVariant &value)
{
	Q_UNUSED(property); Q_UNUSED(value);
	qWarning("%s(): invalid attribute '%s'", __FUNCTION__, qPrintable(attribute));
}


/*!
    \reimp
*/
void QtCursorPropertyManager::initializeProperty(QtProperty *property)
{
#ifndef QT_NO_CURSOR
    d_ptr->m_values[property] = QCursor();
#endif
}

/*!
    \reimp
*/
void QtCursorPropertyManager::uninitializeProperty(QtProperty *property)
{
    d_ptr->m_values.remove(property);
}

#if QT_VERSION >= 0x040400
QT_END_NAMESPACE
#endif

