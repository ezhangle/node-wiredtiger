/*-
 * Copyright (c) 2014- WiredTiger, Inc.
 *	All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

#include <node.h>
#include <string.h>
#include <stdlib.h> // For malloc
#include "NodeWiredTiger.hpp"

using namespace v8;

namespace wiredtiger {

static Persistent<FunctionTemplate> wtconnection_constructor;

WTConnection::WTConnection(char *home, char *config) :
    home_(home), config_(config) {
	conn_ = NULL;
}

WTConnection::~WTConnection() {
	if (conn_ != NULL)
		conn_->close(conn_, NULL);
	if (home_ != NULL)
		free((void *)home_);
	if (config_ != NULL)
		free((void *)config_);
}

WT_CONNECTION * WTConnection::conn() const { return conn_; }
const char * WTConnection::home() const { return home_; }
const char * WTConnection::config() const { return config_; }

/* Calls from worker threads. */

int WTConnection::OpenConnection(const char *home, const char *config) {
	fprintf(stderr, "In WTConnection::OpenConnection\n");
	return (wiredtiger_open(home, NULL, config, &conn_));
}

/* V8 exposed functions */
Handle<Value> WiredTiger(const Arguments& args) {
	HandleScope scope;

	Local<String> home;
	Local<String> config;
	if (args.Length() < 2 || args.Length() > 3 || !args[0]->IsString())
		NODE_WT_THROW_EXCEPTION(
		    "Constructor requires a home argument");
	home = args[0].As<String>();
	if (args.Length() == 2) {
		if (!args[1]->IsString())
			NODE_WT_THROW_EXCEPTION(
			    "Constructor option must be a string");
		config = args[1].As<String>();
	}
	return scope.Close(WTConnection::NewInstance(home, config));
}

void WTConnection::Init(Handle<Object> target) {
	Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
	Local<String> name = String::NewSymbol("WTConnection");

	fprintf(stderr, "Calling connection init\n");
	tpl->SetClassName(name);
	tpl->InstanceTemplate()->SetInternalFieldCount(2);
	wtconnection_constructor = Persistent<FunctionTemplate>::New(tpl);

	NODE_SET_PROTOTYPE_METHOD(tpl, "Open", WTConnection::Open);

	target->Set(name, wtconnection_constructor->GetFunction());
}

Handle<Value> WTConnection::New(const Arguments &args) {
	HandleScope scope;

	if (args.Length() == 0 || !args[0]->IsString())
		NODE_WT_THROW_EXCEPTION(
		    "constructor requires a home argument");
	Local<String> appConfig = String::New(
	    "async=(enabled=true,threads=10,ops_max=4096),"
	    "extensions=[lib/libwiredtiger_zlib.so,"
	    "lib/libwiredtiger_bzip2.so,lib/libwiredtiger_snappy.so],");
	char *home = strdup(*String::Utf8Value(args[0].As<String>()));
	if (args.Length() == 2) {
		if (!args[1]->IsString())
			NODE_WT_THROW_EXCEPTION(
			    "Constructor option must be a string");
		appConfig = String::Concat(
		    appConfig, args[1].As<String>());
	}

	char *config = strdup(*String::Utf8Value(appConfig));
	WTConnection *conn = new WTConnection(home, config);
	conn->Wrap(args.This());
	conn->Ref();

	return scope.Close(args.This());
}

Handle<Value> WTConnection::NewInstance(
    Local<String> &home, Local<String> &config) {

	HandleScope scope;
	Local<Object> instance;

	/* Copy into a local scope handle. */
	Local<FunctionTemplate> constructorHandle =
	    Local<FunctionTemplate>::New(wtconnection_constructor);

	if (home.IsEmpty())
		NODE_WT_THROW_EXCEPTION("constructor requires home argument");

	if (config.IsEmpty()) {
		Handle<Value> argv[] = { home };
		instance =
		    constructorHandle->GetFunction()->NewInstance(1, argv);
	} else {
		Handle<Value> argv[] = { home, config };
		instance =
		    constructorHandle->GetFunction()->NewInstance(2, argv);
	}
	return scope.Close(instance);
}

Handle<Value> WTConnection::Open(const Arguments& args) {
	HandleScope scope;
	if (args.Length() == 0 || !args[0]->IsFunction())
		NODE_WT_THROW_EXCEPTION(
		    "Open() requires a callback argument");
	wiredtiger::WTConnection *conn =
	    node::ObjectWrap::Unwrap<wiredtiger::WTConnection>(args.This());
	Local<Function> callback = args[0].As<Function>();

	ConnectionWorker *worker = new ConnectionWorker(
	    Persistent<Function>::New(callback),
	    Persistent<Object>::New(args.This()),
	    conn, conn->home(), conn->config());

	worker->Queue();
	return scope.Close(Undefined());
}
}
