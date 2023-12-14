#include "database_connection.h"

#include "database.h"
#include "query_builder.h"
#include "query_result.h"
#include "table_builder.h"

void DatabaseConnection::database_connect(const String &connection_str) {
}

Ref<QueryResult> DatabaseConnection::query(const String &query) {
	return Ref<QueryResult>();
}
void DatabaseConnection::query_run(const String &query) {
}

Ref<QueryBuilder> DatabaseConnection::get_query_builder() {
	return Ref<QueryBuilder>(new QueryBuilder());
}

Ref<TableBuilder> DatabaseConnection::get_table_builder() {
	return Ref<TableBuilder>(new TableBuilder());
}

String DatabaseConnection::escape(const String &str) {
	return String();
}

void DatabaseConnection::escape_to(const String &str, String *to) {
}

Ref<Database> DatabaseConnection::get_owner() {
	return Ref<Database>(_owner);
}

int DatabaseConnection::get_table_version(const String &table) {
	ensure_version_table_exists();

	//get_query_builder()
	//TODO

	return 0;
}
void DatabaseConnection::set_table_version(const String &table, const int version) {
	//TODO
}
void DatabaseConnection::ensure_version_table_exists() {
	//TODO
}

void DatabaseConnection::set_owner(Database *owner) {
	_owner = owner;
}

DatabaseConnection::DatabaseConnection() {
	_owner = nullptr;
}

DatabaseConnection::~DatabaseConnection() {
	_owner = nullptr;
}

void DatabaseConnection::_bind_methods() {
	ClassDB::bind_method(D_METHOD("database_connect", "connection_str"), &DatabaseConnection::database_connect);
	ClassDB::bind_method(D_METHOD("query", "query"), &DatabaseConnection::query);
	ClassDB::bind_method(D_METHOD("query_run", "query"), &DatabaseConnection::query_run);

	ClassDB::bind_method(D_METHOD("get_query_builder"), &DatabaseConnection::get_query_builder);
	ClassDB::bind_method(D_METHOD("get_table_builder"), &DatabaseConnection::get_table_builder);

	ClassDB::bind_method(D_METHOD("escape", "str"), &DatabaseConnection::escape);

	ClassDB::bind_method(D_METHOD("get_table_version", "table"), &DatabaseConnection::get_table_version);
	ClassDB::bind_method(D_METHOD("set_table_version", "table", "version"), &DatabaseConnection::set_table_version);
	ClassDB::bind_method(D_METHOD("ensure_version_table_exists"), &DatabaseConnection::ensure_version_table_exists);

	ClassDB::bind_method(D_METHOD("get_owner"), &DatabaseConnection::get_owner);
}
