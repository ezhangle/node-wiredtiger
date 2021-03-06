/*-
 * Copyright (c) 2014- WiredTiger, Inc.
 *	All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

#ifndef NODE_WIREDTIGER_H
#define NODE_WIREDTIGER_H

#include <node.h>
#include "AsyncWorkers.hpp"
#include "wiredtiger.h"

namespace wiredtiger {

	v8::Handle<v8::Value> WiredTiger(const v8::Arguments& args);

class WTConnection : public node::ObjectWrap {
public:
	static void Init(v8::Handle<v8::Object> target);
	static v8::Handle<v8::Value> NewInstance(
	    v8::Local<v8::String> &home, v8::Local<v8::String> &config);
	int OpenConnection(const char *home, const char *config);

	WTConnection(char *home, char *config);
	~WTConnection();

	const char *home() const;
	const char *config() const;
	WT_CONNECTION *conn() const;

private:
	static v8::Handle<v8::Value> New(const v8::Arguments& args);
	static v8::Handle<v8::Value> OpenTable(const v8::Arguments& args);
	static v8::Handle<v8::Value> Open(const v8::Arguments& args);
	static v8::Handle<v8::Value> Put(const v8::Arguments& args);
	static v8::Handle<v8::Value> Search(const v8::Arguments& args);

	char *home_;
	char *config_;
	WT_CONNECTION *conn_;
};

class WTTable : public node::ObjectWrap {
public:
	static void Init(v8::Handle<v8::Object> target);
	static v8::Handle<v8::Value> NewInstance(
	    v8::Local<v8::Object> &wtconn,
	    v8::Local<v8::String> &uri,
	    v8::Local<v8::String> &config);

	WTTable(WTConnection *wtconn, const char *uri, const char *config);
	~WTTable();

	int OpenTable();
	v8::Persistent<v8::Function> Emit;
	WTConnection *wtconn() const;
	const char *uri() const;
	const char *config() const;

private:
	static v8::Handle<v8::Value> New(const v8::Arguments& args);
	static v8::Handle<v8::Value> Open(const v8::Arguments& args);
	static v8::Handle<v8::Value> Put(const v8::Arguments& args);
	static v8::Handle<v8::Value> Search(const v8::Arguments& args);

	WTConnection *wtconn_;
	const char *uri_;
	const char *config_;
};

class WTCursor : public node::ObjectWrap {
public:
	static void Init(v8::Handle<v8::Object> target);
	static v8::Handle<v8::Value> NewInstance(
	    v8::Local<v8::Object> &wttable,
	    v8::Local<v8::String> &config);

	WTCursor(WTTable *wttable, const char *config);
	~WTCursor();

	int CloseImpl();
	int NextImpl(char **keyp, char **valuep);
	int SearchImpl(char *key, char **valuep);

	int startCursorOp(v8::Handle<v8::Function> callback);
	v8::Persistent<v8::Function> Emit;
	WTTable *wttable() const;
	const char *config() const;

private:
	static v8::Handle<v8::Value> Close(const v8::Arguments& args);
	static v8::Handle<v8::Value> New(const v8::Arguments& args);
	static v8::Handle<v8::Value> Next(const v8::Arguments& args);
	static v8::Handle<v8::Value> Search(const v8::Arguments& args);

	WTTable *wttable_;
	WT_CURSOR *cursor_;
	WT_SESSION *session_;
	const char *uri_;
	const char *config_;
	int inOp;
};

#define	NODE_WT_THROW_EXCEPTION_WTERR(msg, err)	do {			\
	HandleScope scope;						\
	v8::ThrowException(						\
	    v8::Exception::Error(v8::String::Concat(			\
	    v8::String::New(msg),					\
	    v8::String::New(wiredtiger_strerror(ret)))));		\
	return Undefined();						\
	} while (0)

#define	NODE_WT_THROW_EXCEPTION(msg)	do {				\
	HandleScope scope;						\
	v8::ThrowException(						\
	    v8::Exception::Error(v8::String::New(msg)));		\
	return Undefined();						\
	} while (0)

} // namespace wiredtiger
#endif
