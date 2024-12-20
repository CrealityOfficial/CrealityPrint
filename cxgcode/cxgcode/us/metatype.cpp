#include "metatype.h"

namespace cxgcode
{
	MetaType::MetaType()
	{
	}

	MetaType::~MetaType()
	{
	}

	StrType sStrType;
	StrType::StrType()
	{

	}

	StrType::~StrType()
	{

	}

	QVariant StrType::value(const QString& str)
	{
		QVariant value;
		value.setValue(str);
		return value;
	}

	FloatType sFloatType;
	FloatType::FloatType()
	{

	}

	FloatType::~FloatType()
	{

	}

	QVariant FloatType::value(const QString& str)
	{
		QVariant value;
		value.setValue(0.0f);

		bool ok = false;
		float f = str.toFloat(&ok);
		if(ok) value.setValue(f);
		
		return value;
	}

	IntType sIntType;
	IntType::IntType()
	{

	}

	IntType::~IntType()
	{

	}

	QVariant IntType::value(const QString& str)
	{
		QVariant value;
		value.setValue(0);

		bool ok = false;
		int i = str.toInt(&ok);
		if (ok) value.setValue(i);

		return value;
	}

	BoolType sBoolType;
	BoolType::BoolType()
	{

	}

	BoolType::~BoolType()
	{

	}

	QVariant BoolType::value(const QString& str)
	{
		bool b = false;

		if ((str == "true") || (str == "TRUE") || (str == "True"))
			b = true;
		QVariant value;
		value.setValue(b);
		return value;
	}

	EnumType sEnumType;
	QVariant EnumType::value(const QString& str)
	{
		QVariant value;
		value.setValue(str);
		return value;
	}

	PolygonType sPolygonType;
	QVariant PolygonType::value(const QString& str)
	{
		QVariant value;
		value.setValue(str);
		return value;
	}

	PolygonsType sPolygonsType;
	QVariant PolygonsType::value(const QString& str)
	{
		QVariant value;
		value.setValue(str);
		return value;
	}

	OptionalExtruderType sOptionalExtruderType;
	QVariant OptionalExtruderType::value(const QString& str)
	{
		QVariant value;
		value.setValue(str);
		return value;
	}

	IntArrayType sIntArrayType;
	QVariant IntArrayType::value(const QString& str)
	{
		QVariant value;
		value.setValue(str);
		return value;
	}

	ExtruderType sExtruderType;
	QVariant ExtruderType::value(const QString& str)
	{
		QVariant value;
		value.setValue(str);
		return value;
	}
}
