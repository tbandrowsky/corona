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
        public string? ClassName { get; protected set;  }
    }

    public class QueryFrom : CoronaBaseObject
    {
        /// <summary>
        /// Class to query from
        /// </summary>
        [JsonProperty("name")]
        public string? Name { get; set; }
        [JsonProperty("filter")]
        public IDictionary<string, string> Filter { get; set; }
        public QueryFrom()
        {
            ClassName = "from";
        }
    }

    public class QueryStage : CoronaBaseObject
    {
        [JsonProperty("name")]
        public string? Name { get; set; }
        [JsonProperty("output")]
        public string? Output { get; set; }

        public QueryStage()
        {
            ClassName = "query_stage";
        }
    }

    public class QueryResult : QueryStage
    {
        public QueryResult()
        {
            ClassName = "result";
        }
    }

    public class Condition : CoronaBaseObject
    {
        
    }

    public class ConditionLt : Condition
    {
        [JsonProperty("value_path")]
        public string? ValuePath { get; set; }
        [JsonProperty("value")]
        public string? Value { get; set; }

        public ConditionLt()
        {
            ClassName = "lt";
        }
    }

    public class ConditionLte : Condition
    {
        [JsonProperty("value_path")]
        public string? ValuePath { get; set; }
        [JsonProperty("value")]
        public string? Value { get; set; }
        public ConditionLte()
        {
            ClassName = "lte";
        }
    }

    public class ConditionEq : Condition
    {
        [JsonProperty("value_path")]
        public string? ValuePath { get; set; }
        [JsonProperty("value")]
        public string? Value { get; set; }

        public ConditionEq()
        {
            ClassName = "eq";
        }

    }

    public class ConditionGte : Condition
    {
        [JsonProperty("value_path")]
        public string? ValuePath { get; set; }
        [JsonProperty("value")]
        public string? Value { get; set; }
        public ConditionGte()
        {
            ClassName = "gte";
        }

    }

    public class ConditionGt : Condition
    {
        [JsonProperty("value_path")]
        public string? ValuePath { get; set; }
        [JsonProperty("value")]
        public string? Value { get; set; }
        public ConditionGt()
        {
            ClassName = "gt";
        }

    }

    public class ConditionContains : Condition
    {
        [JsonProperty("value_path")]
        public string? ValuePath { get; set; }
        [JsonProperty("value")]
        public string? Value { get; set; }
        public ConditionContains()
        {
            ClassName = "contains";
        }

    }

    public class ConditionIn : Condition
    {
        [JsonProperty("value_path")]
        public string? ValuePath { get; set; }
        [JsonProperty("value")]
        public string? Value { get; set; }

        public ConditionIn()
        {
            ClassName = "in";
        }

    }

    public class ConditionBetween : Condition
    {
        [JsonProperty("start")]
        public ConditionGte? Start { get; set; }
        [JsonProperty("stop")]
        public ConditionLte? End { get; set; }
        public ConditionBetween()
        {
            ClassName = "between";
        }

    }

    public class ConditionAllowAll : Condition
    {
        [JsonProperty("conditions")]
        public IList<Condition>? Conditions { get; set; }

        public ConditionAllowAll()
        {
            ClassName = "allow_all";
        }
    }

    public class ConditionAny: Condition
    {
        [JsonProperty("conditions")]
        public IList<Condition>? Conditions { get; set; }
    }

    public class ConditionAll : Condition
    {
        [JsonProperty("conditions")]
        public IList<Condition>? Conditions { get; set; }

        public ConditionAll()
        {
            ClassName = "all";
        }
    }

    public class ConditionNone : Condition
    {
        [JsonProperty("conditions")]
        public IList<Condition>? Conditions { get; set; }
        public ConditionNone()
        {
            ClassName = "none";
        }
    }

    public class QueryFilter : QueryStage
    {
        [JsonProperty("input")]
        public string? Input { get; set; }
        [JsonProperty("condition")]
        public Condition? Condition { get; set; }
        public QueryFilter()
        {
            ClassName = "filter";
        }

    }

    public class QueryProject : QueryStage
    {
        [JsonProperty("input")]
        public string? Input { get; set; }

        [JsonProperty("projection")]
        public Dictionary<string, string>? Projection { get; set; }

        public QueryProject()
        {
            ClassName = "project";
        }

    }

    public class QueryJoin : QueryStage
    {
        [JsonProperty("resultname1")]
        public string? ResultName1 { get; set; }
        [JsonProperty("resultname2")]
        public string? ResultName2 { get; set; }
        [JsonProperty("source1")]
        public string? Source1 { get; set; }
        [JsonProperty("source2")]
        public string? Source2 { get; set; }
        [JsonProperty("keys")]
        IList<string>? Keys { get; set; }

        public QueryJoin()
        {
            ClassName = "join";
        }
    }

    public class QueryBody
    {
        [JsonProperty("from")]

        public IList<QueryFrom>? From { get; set; } = new List<QueryFrom>();

        [JsonProperty("stages")]
        public IList<QueryStage>? Stages { get; set; } = new List<QueryStage>();

    }

    public class FieldBase
	{
        [JsonProperty("field_name")]
        public string? FieldName { get; set; }
        [JsonProperty("field_type")]
        FieldTypes FieldType { get; set; }
        [JsonProperty("field_description")]
        public string? FieldDescription { get; set; }
        [JsonProperty("required")]
        public bool Required { get; set; }
        [JsonProperty("format")]
        public string? Format { get; set; }
        [JsonProperty("input_mask")]
        public string? InputMask { get; set; }
        [JsonProperty("label")]
        public string? Label { get; set; }
        [JsonProperty("description")]
        public string? Description { get; set; }
        [JsonProperty("grid_row")]
        public string? GridRow { get; set; }
        [JsonProperty("grid_column")]
        public string? GridColumn { get; set; }
        [JsonProperty("display")]
        public string? Display { get; set; }
        [JsonProperty("tab_index")]
        public string? TabIndex { get; set; }
        [JsonProperty("read_only")]
        public bool? ReadOnly { get; set; }
        [JsonProperty("server_only")]
        public bool? ServerOnly { get; set; }
    }

    public class StringField : FieldBase
    {
        [JsonProperty("maximum_length")]
        public int? MaximumLength { get; set; }
        [JsonProperty("minimum_length")]
        public int? MinimumLength { get; set; }
        [JsonProperty("match_pattern")]
        public string? MatchPattern { get; set; }
        [JsonProperty("allowed_values")]
        public IList<string>? AllowedValues { get; set; }
    }

    public class DoubleField : FieldBase
    {
        [JsonProperty("min_value")]
        public double? MinValue { get; set; }
        [JsonProperty("max_value")]
        public double? MaxValue { get; set; }
    }

    public class DateTimeField : FieldBase
    {
        [JsonProperty("min_value")]
        public DateTime? MinValue { get; set; }
        [JsonProperty("max_value")]
        public DateTime? MaxValue { get; set; }
    }

    public class Int64Field : FieldBase
    {
        [JsonProperty("min_value")]
        public long? MinValue { get; set; }
        [JsonProperty("max_value")]
        public long? MaxValue { get; set; }
    }

    public class QueryField : FieldBase
    {
        [JsonProperty("query_body")]
        public QueryBody? QueryBody { get; set; }
    }

    public class ChildBridge
    {
        [JsonProperty("child_class_name")]
        public string? ChildClassName { get; set; }
        [JsonProperty("copy_values")]
        public IDictionary<string, string>? CopyValues { get; set; }
    }

    public class ChildBridges
    {
        public IDictionary<string, ChildBridge>? BaseClasses { get; set; }
        public IDictionary<string, ChildBridge>? AllClasses { get; set; }
    }

    public class ArrayField : FieldBase
    {
        [JsonProperty("fundamental_type")]
        public FieldTypes? FundamentalType { get; set; }
        [JsonProperty("child_objects")]
        public ChildBridges? ChildBridges {  get; set; }
    }

    public class ObjectField : FieldBase
    {

        [JsonProperty("child_objects")]
        public ChildBridge? ChildBridge { get; set; }
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
        public string? IndexName { get; set; }
        [JsonProperty("index_keys")]
        public IList<string>? IndexKeys { get; set; }
    }

    public class SqlFieldMapping
    {
        [JsonProperty("corona_field_name")]
        public string? CoronaFieldName { get; set; }

        [JsonProperty("sql_field_name")]
        public string? SqlFieldName { get; set; }

        [JsonProperty("primary_key")]
        public bool? PrimaryKey { get; set; }

        [JsonProperty("field_type")]
        public FieldTypes? FieldType { get; set; }

        [JsonProperty("string_size")]
        public int? StringSize { get; set; }

        [JsonProperty("is_expression")]
        public bool? IsExpression { get; set; }

        [JsonProperty("field_id")]
        public int? FieldId { get; set; }

    }

    public class SqlIntegration
    {
        [JsonProperty("connection_name")]
        public string? ConnectionName { get; set; }

        [JsonProperty("sql_table_name")]
        public string? TableName { get; set; }
        public IDictionary<string, string> Mappings { get; set; }
    }

    public class CoronaClass
    {
        [JsonProperty("class_name")]
        public string? ClassName { get; set; }
        [JsonProperty("class_description")]
        public string? ClassDescription { get; set; }
        [JsonProperty("class_color")]
        public string? ClassColor { get; set; }

        [JsonProperty("grid_template_rows")]
        public string? GridTemplateRows { get; set; }
        [JsonProperty("grid_template_columns")]
        public string? GridTemplateColumns { get; set; }
        [JsonProperty("class_author")]
        public string? ClassAuthor { get; set; }
        [JsonProperty("class_version")]
        public string? ClassVersion { get; set; }
        [JsonProperty("card_title")]
        public string? CardTitle { get; set; }
        [JsonProperty("card_fields")]
        public IList<string>? CardFields { get; set; }

        [JsonProperty("base_class_name")]
        public string? BaseClassName { get; set; }

        [JsonProperty("parents")]
        public IList<string>? Parents { get; set; }

        [JsonProperty("full_text")]
        public IList<string>? FullText { get; set; }
        [JsonProperty("ancestors")]
        public IList<string>? Ancestors { get; set; }
        [JsonProperty("descendants")]
        public IList<string>? Descendants { get; set; }
        [JsonProperty("fields")]
        public IDictionary<string, FieldBase>? Fields { get; set; }
        [JsonProperty("indexes")]
        public IDictionary<string, Index>? Indexes { get; set; }
        [JsonProperty("sql")]
        public IDictionary<string, SqlIntegration>? Sql { get; set; }

    }

    public class SysObject : CoronaBaseObject
    {
        [JsonProperty("object_id")]
        public string? ObjectId { get; set; }
        [JsonProperty("created")]
        public DateTime? Created { get; set; }
        [JsonProperty("created_by")]
        public string? CreatedBy { get; set; }
        [JsonProperty("updated")]
        public DateTime? Modified { get; set; }
        [JsonProperty("modified_by")]
        public string? ModifiedBy { get; set; }
        [JsonProperty("team")]
        public string? Team { get; set; }
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
        public IList<string>? GrantClasses { get; set; }

        [JsonProperty("get")]
        public  string? GetPermission { get; set; }
        [JsonProperty("put")]
        public  string? PutPermission { get; set; }
        [JsonProperty("delete")]
        public  string? DeletePermission { get; set; }
        [JsonProperty("alter")]
        public  string? AlterPermission { get; set; }
        [JsonProperty("derive")]
        public  string? DerivePermission { get; set; }
    }

    public class SysTeam : CoronaBaseObject
    {
        [JsonProperty("team_name")]
        public string? TeamName { get; set; }

        [JsonProperty("team_description")]
        public string? TeamDescription { get; set; }

        [JsonProperty("team_domain")]
        public string? TeamDomain { get; set; }

        [JsonProperty("permissions")]
        public IList<SysGrant>? Permissions { get; set; }

        [JsonProperty("inventory_classes")]
        public IList<string>? InventoryClasses { get; set; }

        [JsonProperty("allowed_teams")]
        public IList<string>? AllowedTeams { get; set; }

        [JsonProperty("tickets")]
        public IList<Ticket>? Tickets { get; set; }

        [JsonProperty("workflow")]
        public IList<Workflow>? Workflow { get; set; }

        [JsonProperty("items")]
        public IList<Item>? Items { get; set; }
    }

    public class SysUser : SysObject
    {
        [JsonProperty("user_name")]
        public string? UserName { get; set; }

        [JsonProperty("email")]
        public string? Email { get; set; }

        [JsonProperty("first_name")]
        public string? FirstName { get; set; }

        [JsonProperty("last_name")]
        public string? LastName { get; set; }

        [JsonProperty("street1")]
        public string? Street1 { get; set; }

        [JsonProperty("street2")]
        public string? Street2 { get; set; }

        [JsonProperty("city")]
        public string? City { get; set; }

        [JsonProperty("state")]
        public string? State { get; set; }

        [JsonProperty("zip")]
        public string? Zip { get; set; }

        [JsonProperty("team_name")]
        public string? TeamName { get; set; }

        [JsonProperty("team")]
        public SysTeam? Team { get; set; }

        [JsonProperty("home_team_name")]
        public string? HomeTeamName { get; set; }

        [JsonProperty("home_team")]
        public SysTeam? HomeTeam { get; set; }

    }

}
