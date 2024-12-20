#ifndef _US_METATYPE_1589463173476_H
#define _US_METATYPE_1589463173476_H
#include "basickernelexport.h"
#include <QtCore/QVariant>

namespace us
{
	class BASIC_KERNEL_API MetaType
	{
	public:
		MetaType();
		virtual ~MetaType();

		virtual QVariant value(const QString& str) = 0;
	protected:
	};

	class StrType : public MetaType
	{
	public:
		StrType();
		virtual ~StrType();

	protected:
		QVariant value(const QString& str) override;
	};

	class FloatType : public MetaType
	{
	public:
		FloatType();
		virtual ~FloatType();

	protected:
		QVariant value(const QString& str) override;
	};

	class IntType : public MetaType
	{
	public:
		IntType();
		virtual ~IntType();

	protected:
		QVariant value(const QString& str) override;
	};

	class BoolType : public MetaType
	{
	public:
		BoolType();
		virtual ~BoolType();

	protected:
		QVariant value(const QString& str) override;
	};

	class EnumType : public MetaType
	{
	protected:
		QVariant value(const QString& str) override;
	};

	class PolygonType : public MetaType
	{
	protected:
		QVariant value(const QString& str) override;
	};

	class PolygonsType : public MetaType
	{
	protected:
		QVariant value(const QString& str) override;
	};

	class OptionalExtruderType : public MetaType
	{
	protected:
		QVariant value(const QString& str) override;
	};

	class IntArrayType : public MetaType
	{
	protected:
		QVariant value(const QString& str) override;
	};

	class ExtruderType : public MetaType
	{
	protected:
		QVariant value(const QString& str) override;
	};

	extern StrType sStrType;
	extern FloatType sFloatType;
	extern IntType sIntType;
	extern BoolType sBoolType;
	extern EnumType sEnumType;
	extern PolygonType sPolygonType;
	extern PolygonsType sPolygonsType;
	extern OptionalExtruderType sOptionalExtruderType;
	extern IntArrayType sIntArrayType;
	extern ExtruderType sExtruderType;
}
#endif // _US_METATYPE_1589463173476_H
