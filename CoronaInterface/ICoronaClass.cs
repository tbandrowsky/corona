using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.ComponentModel;
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

    public class CoronaBaseObject
    {
        [JsonProperty("class_name")]
        string ClassName { get; set;  }
    }

    public class QueryFrom : CoronaBaseObject
    {
        [JsonProperty("name")]
        string Name { get; set; }
        [JsonProperty("filter")]
        IDictionary<string, string> Filter { get; set; }
    }

    public class QueryStage : CoronaBaseObject
    {
        [JsonProperty("name")]
        string? Name { get; set; }
        [JsonProperty("output")]
        string? Output { get; set; }
    }

    public class QueryResult : QueryStage
    {
    }

    public class Condition : CoronaBaseObject
    {
        
    }

    public class ConditionLt : Condition
    {
        [JsonProperty("value_path")]
        string? ValuePath { get; set; }
        [JsonProperty("value")]
        string? Value { get; set; }
    }

    public class ConditionLte : Condition
    {
        [JsonProperty("value_path")]
        string? ValuePath { get; set; }
        [JsonProperty("value")]
        string? Value { get; set; }
    }

    public class ConditionEq : Condition
    {
        [JsonProperty("value_path")]
        string? ValuePath { get; set; }
        [JsonProperty("value")]
        string? Value { get; set; }
    }

    public class ConditionGte : Condition
    {
        [JsonProperty("value_path")]
        string? ValuePath { get; set; }
        [JsonProperty("value")]
        string? Value { get; set; }
    }

    public class ConditionGt : Condition
    {
        [JsonProperty("value_path")]
        string? ValuePath { get; set; }
        [JsonProperty("value")]
        string? Value { get; set; }
    }

    public class ConditionContains : Condition
    {
        [JsonProperty("value_path")]
        string? ValuePath { get; set; }
        [JsonProperty("value")]
        string? Value { get; set; }
    }

    public class ConditionIn : Condition
    {
        [JsonProperty("value_path")]
        string? ValuePath { get; set; }
        [JsonProperty("value")]
        string? Value { get; set; }
    }

    public class ConditionBetween : Condition
    {
        [JsonProperty("start")]
        ConditionGte? Start { get; set; }
        [JsonProperty("stop")]
        ConditionLte? End { get; set; }
    }

    public class ConditionAllowAll : Condition
    {
        [JsonProperty("conditions")]
        IList<Condition>? Conditions { get; set; }
    }

    public class ConditionAny: Condition
    {
        [JsonProperty("conditions")]
        IList<Condition>? Conditions { get; set; }
    }

    public class ConditionAll : Condition
    {
        [JsonProperty("conditions")]
        IList<Condition>? Conditions { get; set; }
    }

    public class ConditionNone : Condition
    {
        [JsonProperty("conditions")]
        IList<Condition>? Conditions { get; set; }
    }

    public class QueryFilter : QueryStage
    {
        [JsonProperty("input")]
        string? Input { get; set; }
        [JsonProperty("condition")]
        Condition? Condition { get; set; }
    }

    public class QueryProject : QueryStage
    {
        [JsonProperty("input")]
        string? Input { get; set; }

        [JsonProperty("projection")]
        Dictionary<string, string>? Projection { get; set; }
    }

    public class QueryJoin : QueryStage
    {
        [JsonProperty("resultname1")]
        string? ResultName1 { get; set; }
        [JsonProperty("resultname2")]
        string? ResultName2 { get; set; }
        [JsonProperty("source1")]
        string? Source1 { get; set; }
        [JsonProperty("source2")]
        string? Source2 { get; set; }
        [JsonProperty("keys")]
        IList<string>? Keys { get; set; }
    }

    public class QueryBody
    {
        [JsonProperty("from")]

        IList<QueryFrom> From { get; set; }

        [JsonProperty("stages")]
        IList<QueryStage> Stages { get; set; }
    }

    public class FieldBase
	{
        [JsonProperty("field_name")]
        string FieldName { get; set; }
        [JsonProperty("field_type")]
        FieldTypes FieldType { get; set; }
        [JsonProperty("field_description")]
        string FieldDescription { get; set; }
        [JsonProperty("required")]
        bool Required { get; set; }
        [JsonProperty("format")]
        string Format { get; set; }
        [JsonProperty("input_mask")]
        string InputMask { get; set; }
        [JsonProperty("label")]
        string Label { get; set; }
        [JsonProperty("description")]
        string Description { get; set; }
        [JsonProperty("grid_row")]
        string GridRow { get; set; }
        [JsonProperty("grid_column")]
        string GridColumn { get; set; }
        [JsonProperty("display")]
        string Display { get; set; }
        [JsonProperty("tab_index")]
        string TabIndex { get; set; }
        [JsonProperty("read_only")]
        bool ReadOnly { get; set; }
        [JsonProperty("server_only")]
        bool ServerOnly { get; set; }
    }

    public class StringField : FieldBase
    {
        [JsonProperty("maximum_length")]
        int MaximumLength { get; set; }
        [JsonProperty("minimum_length")]
        int MinimumLength { get; set; }
        [JsonProperty("match_pattern")]
        string MatchPattern { get; set; }
        [JsonProperty("allowed_values")]
        IList<string> AllowedValues { get; set; }
    }

    public class DoubleField : FieldBase
    {
        [JsonProperty("min_value")]
        double MinValue { get; set; }
        [JsonProperty("max_value")]
        double MaxValue { get; set; }
    }

    public class DateTimeField : FieldBase
    {
        [JsonProperty("min_value")]
        DateTime MinValue { get; set; }
        [JsonProperty("max_value")]
        DateTime MaxValue { get; set; }
    }

    public class Int64Field : FieldBase
    {
        [JsonProperty("min_value")]
        Int64 MinValue { get; set; }
        [JsonProperty("max_value")]
        Int64 MaxValue { get; set; }
    }

    public class QueryField : FieldBase
    {
        [JsonProperty("query_body")]
        QueryBody QueryBody { get; set; }
    }

    public class ChildBridge
    {
        [JsonProperty("child_class_name")]
        string ChildClassName { get; set; }
        [JsonProperty("copy_values")]
        IDictionary<string, string> CopyValues { get; set; }
    }

    public class ChildBridges
    {
        IDictionary<string, ChildBridge> BaseClasses { get; set; }
        IDictionary<string, ChildBridge> AllClasses { get; set; }
    }

    public class ArrayField : FieldBase
    {
        [JsonProperty("fundamental_type")]
        FieldTypes FundamentalType { get; set; }
        [JsonProperty("child_objects")]
        ChildBridges ChildBridges {  get; set; }
    }

    public class ObjectField : FieldBase
    {

        [JsonProperty("child_objects")]
        ChildBridge ChildBridge { get; set; }
    }

    public class FunctionField : FieldBase
    {
    }

    public class ReferenceField : FieldBase 
    {
    }

    public class Index
	{

        [JsonProperty("index_name")]
        string IndexName { get; set; }
        [JsonProperty("index_keys")]
        IList<string> IndexKeys { get; set; }
    }

    public class SqlFieldMapping
    {
        [JsonProperty("corona_field_name")]
        string CoronaFieldName { get; set; }

        [JsonProperty("sql_field_name")]
        string SqlFieldName { get; set; }

        [JsonProperty("primary_key")]
        bool PrimaryKey { get; set; }

        [JsonProperty("field_type")]
        FieldTypes FieldType { get; set; }

        [JsonProperty("string_size")]
        int StringSize { get; set; }

        [JsonProperty("is_expression")]
        bool IsExpression { get; set; }

        [JsonProperty("field_id")]
        int FieldId { get; set; }

    }

    public class SqlIntegration
    {
        [JsonProperty("connection_name")]
        string ConnectionName { get; set; }

        [JsonProperty("sql_table_name")]
        string TableName { get; set; }
        IDictionary<string, string> Mappings { get; set; }
    }

    public class CoronaClass
    {
        [JsonProperty("class_name")]
        string ClassName { get; set; }
        [JsonProperty("class_description")]
        string ClassDescription { get; set; }
        [JsonProperty("class_color")]
        string ClassColor { get; set; }

        [JsonProperty("grid_template_rows")]
        string GridTemplateRows { get; set; }
        [JsonProperty("grid_template_columns")]
        string GridTemplateColumns { get; set; }
        [JsonProperty("class_author")]
        string ClassAuthor { get; set; }
        [JsonProperty("class_version")]
        string ClassVersion { get; set; }
        [JsonProperty("card_title")]
        string CardTitle { get; set; }
        [JsonProperty("card_fields")]
        IList<string> CardFields { get; set; }

        [JsonProperty("base_class_name")]
        string BaseClassName { get; set; }

        [JsonProperty("parents")]
        IList<string> Parents { get; set; }

        [JsonProperty("full_text")]
        IList<string> FullText { get; set; }
        [JsonProperty("ancestors")]
        IList<string> Ancestors { get; set; }
        [JsonProperty("descendants")]
        IList<string> Descendants { get; set; }
        [JsonProperty("fields")]
        IDictionary<string, FieldBase> Fields { get; set; }
        [JsonProperty("indexes")]
        IDictionary<string, Index> Indexes { get; set; }
        [JsonProperty("sql")]
        IDictionary<string, SqlIntegration> Sql { get; set; }

    }

    public class SysObject : CoronaBaseObject
    {
        [JsonProperty("object_id")]
        string ObjectId { get; set; }
        [JsonProperty("created")]
        DateTime Created { get; set; }
        [JsonProperty("created_by")]
        string CreatedBy { get; set; }
        [JsonProperty("updated")]
        DateTime Modified { get; set; }
        [JsonProperty("modified_by")]
        string ModifiedBy { get; set; }
        [JsonProperty("team")]
        string Team { get; set; }
    }

    public class Ticket : SysObject
    {
    }
    public class Workflow : SysObject
    {
    }
    public class Item : SysObject
    {
    }

    public class SysGrant : CoronaBaseObject
    {
        [JsonProperty("grant_classes")]
        IList<string> GrantClasses { get; set; }

        [JsonProperty("get")]
        string GetPermission { get; set; }
        [JsonProperty("put")]
        string PutPermission { get; set; }
        [JsonProperty("delete")]
        string DeletePermission { get; set; }
        [JsonProperty("alter")]
        string AlterPermission { get; set; }
        [JsonProperty("derive")]
        string DerivePermission { get; set; }
    }

    public class SysTeam : CoronaBaseObject
    {
        [JsonProperty("team_name")]
        string TeamName { get; set; }

        [JsonProperty("team_description")]
        string TeamDescription { get; set; }

        [JsonProperty("team_domain")]
        string TeamDomain { get; set; }

        [JsonProperty("permissions")]
        IList<ISysGrant> Permissions { get; set; }

        [JsonProperty("inventory_classes")]
        IList<string> InventoryClasses { get; set; }

        [JsonProperty("allowed_teams")]
        IList<string> AllowedTeams { get; set; }

        [JsonProperty("tickets")]
        IList<Ticket> Tickets { get; set; }

        [JsonProperty("workflow")]
        IList<Workflow> Workflow { get; set; }

        [JsonProperty("items")]
        IList<Item> Items { get; set; }
    }

    public class SysUser : SysObject
    {
        [JsonProperty("user_name")]
        public string? UserName { get; set; }

        [JsonProperty("email")]
        public string Email { get; set; }

        [JsonProperty("first_name")]
        public string FirstName { get; set; }

        [JsonProperty("last_name")]
        public string LastName { get; set; }

        [JsonProperty("street1")]
        public string Street1 { get; set; }

        [JsonProperty("street2")]
        public string Street2 { get; set; }

        [JsonProperty("city")]
        public string City { get; set; }

        [JsonProperty("state")]
        public string State { get; set; }

        [JsonProperty("zip")]
        public string Zip { get; set; }
    }

}
