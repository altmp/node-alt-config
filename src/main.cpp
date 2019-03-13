#include <node_api.h>
#include <assert.h>
#include "alt-config.h"

#define DECLARE_NAPI_METHOD(name, func) \
  { name, 0, func, 0, 0, 0, napi_default, 0 }

typedef struct {
	napi_async_work _request;
	napi_deferred deferred;

	char* error;
	char* string;
	int* line;
	int* column;
	alt::config::Node* result;
} async_parse_data;

napi_value parseNode(napi_env env, alt::config::Node node) {
	napi_value value;
	napi_status status;
	if (node.IsDict()) {
		status = napi_create_object(env, &value);
		assert(status == napi_ok);
		auto dict = node.ToDict();
		for (auto &propertyNode : dict) {
			napi_value propertyKey, propertyValue;

			std::string propertyKeyString = propertyNode.first;

			status = napi_create_string_utf8(env, propertyKeyString.c_str(), propertyKeyString.length(), &propertyKey);
			assert(status == napi_ok);

			propertyValue = parseNode(env, propertyNode.second);

			status = napi_set_property(env, value, propertyKey, propertyValue);
			assert(status == napi_ok);
		}
	}
	else if (node.IsList()) {
		status = napi_create_array(env, &value);
		assert(status == napi_ok);
		auto list = node.ToList();
		int i = 0;
		for (auto &elementNode : list) {
			napi_value elementValue;

			elementValue = parseNode(env, elementNode);

			status = napi_set_element(env, value, i++, elementValue);
			assert(status == napi_ok);
		}
	}
	else if (node.IsScalar()) {
		std::string str = node.ToString();
		status = napi_create_string_utf8(env, str.c_str(), str.length(), &value);
		assert(status == napi_ok);
	}
	else {
		status = napi_get_null(env, &value);
		assert(status == napi_ok);
	}
	return value;
}


void doParse(napi_env env, void* param) {
	using namespace alt;

	async_parse_data* data = static_cast<async_parse_data*>(param);

	try {
		config::Parser parser = config::Parser((const char*)data->string, strlen(data->string));
		data->result = new config::Node(parser.Parse());
	}
	catch (const config::Error& e) {
		const char* error = e.what();
		data->error = (char*)malloc(strlen(error) + 1);
		memcpy(data->error, error, strlen(error) + 1);
		data->line = new int(e.line());
		data->column = new int(e.column());
	}
}

void doneParse(napi_env env, napi_status status, void* param) {
	using namespace alt;
	async_parse_data* data = static_cast<async_parse_data*>(param);

	status = napi_delete_async_work(env, data->_request);
	assert(status == napi_ok);

	napi_value value;

	if (data->error == NULL) {
		try {
			value = parseNode(env, *data->result);
		}
		catch (const config::Error e) {
			const char* error = e.what();
			data->error = (char*)malloc(strlen(error) + 1);
			memcpy(data->error, error, strlen(error) + 1);
			data->line = new int(e.line());
			data->column = new int(e.column());
		}
	}

	if (data->error == NULL) {
		status = napi_resolve_deferred(env, data->deferred, value);
	}
	else {
		napi_value errorMessage;

		std::string errorMessageString = "Parsing failed on line " + std::to_string(*data->line) + ", column " + std::to_string(*data->column) + "! Reason: " + data->error; // TODO: Rework, it's bad

		status = napi_create_string_utf8(env, errorMessageString.c_str(), errorMessageString.length(), &errorMessage);
		assert(status == napi_ok);

		status = napi_create_error(env, 0, errorMessage, &value);
		assert(status == napi_ok);

		status = napi_reject_deferred(env, data->deferred, value);
	}
	assert(status == napi_ok);

	free(data);
}


napi_value Parse(napi_env env, napi_callback_info info) {
	napi_status status;

	size_t argc = 1;
	napi_value args[1];
	status = napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
	assert(status == napi_ok);

	if (argc < 1) {
		napi_throw_type_error(env, nullptr, "Wrong number of arguments");
		return nullptr;
	}

	napi_valuetype valuetype0;
	status = napi_typeof(env, args[0], &valuetype0);
	assert(status == napi_ok);

	if (valuetype0 != napi_string) {
		napi_throw_type_error(env, nullptr, "Wrong arguments");
		return nullptr;
	}

	async_parse_data* data = (async_parse_data*)malloc(sizeof(async_parse_data));

	size_t stringLength;
	status = napi_get_value_string_utf8(env, args[0], NULL, 0, &stringLength);
	assert(status == napi_ok);

	data->string = (char*)malloc(stringLength + 1);
	assert(data->string != NULL);

	data->error = NULL;

	status = napi_get_value_string_utf8(env, args[0], data->string, stringLength + 1, &stringLength);
	assert(status == napi_ok);

	napi_value resourceName;
	status = napi_create_string_utf8(env, "parseAsync", -1, &resourceName);
	assert(status == napi_ok);

	napi_value promise;
	status = napi_create_promise(env, &data->deferred, &promise);
	assert(status == napi_ok);

	status = napi_create_async_work(env, 0, resourceName, doParse, doneParse, data, &data->_request);
	assert(status == napi_ok);

	status = napi_queue_async_work(env, data->_request);
	assert(status == napi_ok);

	return promise;
}

napi_value Init(napi_env env, napi_value exports) {
	napi_status status;


	napi_property_descriptor desc[] = {
		DECLARE_NAPI_METHOD("parse", Parse)
	};
	status = napi_define_properties(env, exports, sizeof(desc) / sizeof(*desc), desc);
	assert(status == napi_ok);
	return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init);