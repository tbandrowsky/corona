using System;
using System.Collections.Generic;
using System.Text;

namespace CoronaInterface
{

	public enum FieldTypes
	{
		String,
		Double,
		Int64,
		Boolean,
        DateTime,
        Query,
		Array,
		Object,
		Function,
		Reference,
		Blob
    }

    public interface IFieldBase
	{
		string FieldName { get; set; }
        FieldTypes FieldType { get; set; }
		string FieldDescription { get; set; }
        bool Required { get; set; }
        string Format { get; set; }
        string InputMask { get; set; }
        string Label { get; set; }
        string Description { get; set; }
        string GridRow { get; set; }
        string GridColumn { get; set; }
        string Display { get; set; }
        string TabIndex { get; set; }
        bool ReadOnly { get; set; }
        bool ServerOnly { get; set; }
    }

    public interface IStringField : IFieldBase
    {
        int MaximumLength { get; set; }
        int MinimumLength { get; set; }
        string MatchPattern { get; set; }
        IList<string> AllowedValues { get; set; }
    }

    public interface IDoubleField : IFieldBase
    {
        double MinValue { get; set; }
        double MaxValue { get; set; }
    }

    public interface IDateTimeField : IFieldBase
    {
        DateTime MinValue { get; set; }
        DateTime MaxValue { get; set; }
    }

    public interface IInt64Field : IFieldBase
    {
        Int64 MinValue { get; set; }
        Int64 MaxValue { get; set; }
    }

    public interface IQueryField : IFieldBase
    {
    }

    public interface IArrayField : IFieldBase
    {
    }

    public interface IObjectField : IFieldBase
    {
    }

    public interface IFunctionField : IFieldBase
    {
    }

    public interface IReferenceField : IFieldBase
    {
    }

    public interface IIndex
	{
		string IndexName { get; set; }
		IList<string> IndexKeys { get; set; }
    }

    public interface ICoronaClass
    {
        string ClassName { get; set; }
        string ClassDescription { get; set; }
		string ClassColor { get; set; }
		string GridTemplateRows { get; set; }
		string GridTemplateColumns { get; set; }
		string ClassAuthor { get; set; }
		string ClassVersion { get; set; }
		string CardTitle { get; set; }
		IList<string> CardFields { get; set; }
        string BaseClassName { get; set; }
		IList<string> Parents { get; set; }
		IList<string> FullText { get; set; }
		IList<string> Ancestors { get; set; }
		IList<string> Descendants { get; set; }

    }
}
