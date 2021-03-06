/*
 * Copyright (c) 2000-2003 moe
 *
 *    This source code is free software; you can redistribute it
 *    and/or modify it in source code form under the terms of the GNU
 *    General Public License as published by the Free Software
 *    Foundation; either version 2 of the License, or (at your option)
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */


#ifndef __VERILOG_HH
#define __VERILOG_HH

#include <stdint.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <string>
#include <iostream>
#include <iomanip>
#include <typeinfo>
#include <set>
#include <map>
#include <list>
#include <vector>
#include <algorithm>

using namespace std;

extern int verilog_parse();
extern FILE*  verilog_input;
extern string verilog_file;
extern string verilog_comment;

//int verilog_parse();
//FILE*  verilog_input;
//string verilog_file;
//string verilog_comment;

namespace moe
{
  class Verilog;
}
extern moe::Verilog* source_;

namespace moe
{
  ////////////////////////////////////////////////////////////////////////
  class Verilog
  {
    public:
    class LineInfo
    {
      string       file_;
      unsigned int line_;
    public:
      LineInfo():
	line_(0)
      {}
      
      void setFile(const string& name){ file_=name; }
      void setLine(unsigned int line) { line_=line; }

      const string& file() const { return file_; }
      unsigned int  line() const { return line_; }
      
      string getInfo() const
      {
	char buf[16];
	snprintf(buf,sizeof(buf),"%u",line_);
	return (file_ + " : " + buf);
      }
    };

    class Callback;
    ////////////////////////////////////////////////////////////////////////
    class Module;
    class Net;
    ////////////////////////////////////////////////////////////////////////
    class Expression
    {
    public:
      virtual ~Expression(){}
      virtual bool isConstant() const { return false; }
      virtual signed calcConstant() const { return 0; }
      virtual unsigned int width() const { return 0; }
      virtual void link(const map<string,Net*>& net,Module* mod,const string& scope) {}
      virtual const Net* net() const { return NULL; }
      
      virtual Expression* clone(const string& hname) const { return NULL; }
      virtual Expression* clone() const { return NULL; }

      virtual void chain(set<const Net*>& ev) const {}
      virtual void chain(set<const Expression*>& ev) const { ev.insert((Expression*)this); }

      virtual void toXML(std::ostream& ostr) const {}
      virtual void toVerilog(std::ostream& ostr) const {}

      enum
      {
	ArithmeticAdd,
	ArithmeticMinus,
	ArithmeticMultiply,
	ArithmeticDivide,
	ArithmeticModulus,
	ArithmeticLeftShift,
	ArithmeticRightShift,
	ArithmeticPower,

	LeftShift,
	RightShift,
	
	LessThan,
	GreaterThan,
	LessEqual,
	GreaterEqual,
	CaseEquality,
	CaseInequality,
	
	LogicalNegation,
	LogicalAND,
	LogicalOR,
	LogicalEquality,
	LogicalInequality,
	
	BitwiseAND,
	BitwiseOR,
	BitwiseNOR,
	BitwiseNXOR,
	BitwiseXOR,
	BitwiseNegation,
	
	ReductionAND,
	ReductionNAND,
	ReductionNOR,
	ReductionNXOR,
	ReductionXOR,
	ReductionOR,

	CastUnsigned,
	CastSigned
      };

      virtual void callback(Callback& cb) const{}
    };
    ////////////////////////////////////////////////////////////////////////
    class String : public Expression
    {
      string text_;
    public:
      String()
      {}
      String(const char* text);
      ~String(){}

      const string& text() const { return text_; }

      void toXML(std::ostream& ostr) const;
      void toVerilog(std::ostream& ostr) const;

      void link(const map<string,Net*>& net,Module* mod,const string& scope);
      Expression* clone(const string& hname) const { return new String(*this); }
      Expression* clone() const { return new String(*this); }

      void callback(Callback& cb) const;
    };
    ////////////////////////////////////////////////////////////////////////
    class Number : public Expression
    {
      string text_;
      string bitset_;

      string value_;
      string mask_;

      unsigned int width_;
    public:
      Number():
      width_(0)
      {}
      Number(const char* text);
      ~Number(){}

      const string& text() const { return text_; }
      const string& bitset() const { return bitset_; }
      unsigned int width() const { return width_; }

      bool isConstant() const { return true; }
      bool isPartial() const;

      const string& value() const { return value_; }
      const string& mask() const { return mask_; }

      signed calcConstant() const;

      void toXML(std::ostream& ostr) const;
      void toVerilog(std::ostream& ostr) const;

      void link(const map<string,Net*>& net,Module* mod,const string& scope);
      Expression* clone(const string& hname) const { return new Number(*this); }
      Expression* clone() const { return new Number(*this); }

      void callback(Callback& cb) const;
    };
    ////////////////////////////////////////////////////////////////////////
    class Identifier : public Expression
    {
      string      name_;
      Expression* msb_;
      Expression* lsb_;
      Expression* idx_;
      Net*        net_;
      //////////////////
    public:
      Identifier(const char* name,
		 Expression* msb=NULL,Expression* lsb=NULL,
		 Expression* idx=NULL):
	name_(name),
	msb_(msb),lsb_(lsb),
	idx_(idx),
	net_(NULL)
      {}
      Identifier():
	msb_(NULL),lsb_(NULL),
	idx_(NULL),
	net_(NULL)
      {}
      ~Identifier();
      const string& name() const { return name_; }
      const Expression* msb() const { return msb_; }
      const Expression* lsb() const { return lsb_; }
      const Expression* idx() const { return idx_; }

      bool isPartial() const;

      signed calcConstant() const
      {
	if( net_!=NULL )
	  return net_->calcConstant();
	else
	  return 0;
      }

      void toXML(std::ostream& ostr) const;
      void toVerilog(std::ostream& ostr) const;

      unsigned int width() const;

      void link(const map<string,Net*>& net,Module* mod,const string& scope);
      const Net* net() const { return net_; }
      void setNet(Net* net) { net_=net; }
      Expression* clone(const string& hname) const;
      Expression* clone() const;

      void chain(set<const Net*>& ev) const;
      void chain(set<const Expression*>& ev) const;

      void callback(Callback& cb) const;
    };
    ////////////////////////////////////////////////////////////////////////
    class Concat : public Expression
    {
      Expression*         repeat_;
      vector<Expression*> list_;
    public:
      Concat(const vector<Expression*>& l):
	repeat_(NULL),
	list_(l)
      {}
      Concat(Expression* r,const vector<Expression*>& l):
	repeat_(r),
	list_(l)
      {}
      Concat():
	repeat_(NULL)
      {}
      ~Concat();
      const Expression* repeat() const { return repeat_; }
      const vector<Expression*>& list() const { return list_; }

      void toXML(std::ostream& ostr) const;
      void toVerilog(std::ostream& ostr) const;

      unsigned int width() const;
      void link(const map<string,Net*>& net,Module* mod,const string& scope);
      Expression* clone(const string& hname) const;
      Expression* clone() const;

      void chain(set<const Net*>& ev) const;
      void chain(set<const Expression*>& ev) const;

      void callback(Callback& cb) const;
    };
    ////////////////////////////////////////////////////////////////////////
    class Event : public Expression
    {
      int         type_;
      Expression* expr_;
    public:
      enum
      {
	ANYEDGE,
	POSEDGE,
	NEGEDGE,
	POSITIVE
      };
      Event(int t,Expression* e):
	type_(t),
	expr_(e)
      {}
      Event():
	expr_(NULL)
      {}
      ~Event();
      int         type() const { return type_; }
      Expression* expression() const { return expr_; }

      void toXML(std::ostream& ostr) const;
      void toVerilog(std::ostream& ostr) const;

      unsigned int width() const { return 0; }
      void link(const map<string,Net*>& net,Module* mod,const string& scope);
      Expression* clone(const string& hname) const;
      Expression* clone() const;

      void chain(set<const Net*>& ev) const;
      void chain(set<const Expression*>& ev) const;

      void callback(Callback& cb) const;
    };
    ////////////////////////////////////////////////////////////////////////
    class Unary : public Expression
    {
      /*
	ArithmeticMinus
	BitwiseNegation
	LogicalNegation
	ReductionAND
	ReductionOR
	ReductionXOR
	ReductionNAND
	ReductionNOR
	ReductionNXOR
	CastSigned
	CastUnsigned
       */
      int         op_;
      Expression* expr_;
    public:
      Unary(int o,Expression* e):
	op_(o),
	expr_(e)
      {}
      Unary():
	expr_(NULL)
      {}
      ~Unary();

      int operation() const { return op_; }
      const Expression* value() const { return expr_; }

      unsigned int width() const;
      bool isConstant() const { return expr_->isConstant(); }
      signed calcConstant() const;

      void toXML(std::ostream& ostr) const;
      void toVerilog(std::ostream& ostr) const;

      const char* opToken() const;
      const char* opName() const;

      void link(const map<string,Net*>& net,Module* mod,const string& scope);
      Expression* clone(const string& hname) const;
      Expression* clone() const;

      void chain(set<const Net*>& ev) const;
      void chain(set<const Expression*>& ev) const;

      void callback(Callback& cb) const;
    };
    ////////////////////////////////////////////////////////////////////////
    class Binary : public Expression
    {
      int         op_;
      Expression* left_;
      Expression* right_;
    public:
      Binary(int o,Expression* l,Expression* r):
	op_(o),
	left_(l),
	right_(r)
      {}
      Binary():
	left_(NULL),
	right_(NULL)
      {}
      ~Binary();

      int operation() const { return op_; }
      const Expression* left() const { return left_; }
      const Expression* right() const { return right_; }

      unsigned int width() const;
      bool isConstant() const { return (left_->isConstant()&&right_->isConstant()); }
      signed calcConstant() const;

      void toXML(std::ostream& ostr) const;
      void toVerilog(std::ostream& ostr) const;

      const char* opToken() const;
      const char* opName() const;

      void link(const map<string,Net*>& net,Module* mod,const string& scope);
      Expression* clone(const string& hname) const;
      Expression* clone() const;

      void chain(set<const Net*>& ev) const;
      void chain(set<const Expression*>& ev) const;

      void callback(Callback& cb) const;
    };
    ////////////////////////////////////////////////////////////////////////
    class Ternary : public Expression
    {
      Expression* expr_;
      Expression* true_;
      Expression* false_;
    public:
      Ternary(Expression* e,Expression* t,Expression* f):
	expr_(e),
	true_(t),
	false_(f)
      {}
      Ternary():
	expr_(NULL),
	true_(NULL),
	false_(NULL)
      {}
      ~Ternary();

      const Expression* condition() const { return expr_; }
      const Expression* trueValue() const { return true_; }
      const Expression* falseValue() const { return false_; }
      
      unsigned int width() const { return (false_!=NULL) ? max( true_->width(),false_->width() ) : true_->width() ; }
      bool isConstant() const{}

      void toXML(std::ostream& ostr) const;
      void toVerilog(std::ostream& ostr) const;

      void link(const map<string,Net*>& net,Module* mod,const string& scope);
      Expression* clone(const string& hname) const;
      Expression* clone() const;

      void chain(set<const Net*>& ev) const;
      void chain(set<const Expression*>& ev) const;

      void callback(Callback& cb) const;
    };
    class Function;
    ////////////////////////////////////////////////////////////////////////
    class CallFunction : public Expression
    {
      string              name_;
      vector<Expression*> parms_;
      Function*           func_;
      Net*                net_;
    public:
      CallFunction(const char* n,const vector<Expression*> &p):
	name_(n),
	parms_(p),
	func_(NULL),
	net_(NULL)
      {}
      CallFunction():
	func_(NULL),
	net_(NULL)
      {}
      ~CallFunction();

      const string name() const { return name_; }
      const vector<Expression*>& parameter() const { return parms_; }
      const Function* function() const { return func_; }
      const Net* net() const { return net_; }

      void toXML(std::ostream& ostr) const;
      void toVerilog(std::ostream& ostr) const;

      unsigned int width() const { return net_->width(); }
      void link(const map<string,Net*>& net,Module* mod,const string& scope);
      Expression* clone(const string& hname) const;
      Expression* clone() const;

      void chain(set<const Net*>& ev) const;
      void chain(set<const Expression*>& ev) const;

      void callback(Callback& cb) const;
    };
    ////////////////////////////////////////////////////////////////////////
    class Net
    {
    public:
      class nrm_
      {
      public:
	char* name;
	Expression* start;
	Expression* end;
	int   type;
	nrm_():
	  name(NULL)
	{}
	~nrm_()
	{
	  delete name;
	  delete start;
	  delete end;
	}
      };
      int    type_;
      int    interface_;
      bool   sign_;
      
      Expression* msb_;
      Expression* lsb_;
      Expression* sa_;
      Expression* ea_;
      bool constant_;
      string name_;

      const Expression* rval_;
    public:
      enum
      {
	IMPLICIT,
	WIRE,
	TRI,
	TRI1,
	SUPPLY0,
	WAND,
	TRIAND,
	TRI0,
	SUPPLY1,
	WOR, 
	TRIOR,
	REG,
	INTEGER,
	REAL,
	PARAMETER,
	FUNCTION,
	DEFINE,
	CONSTANT,

	NAMEDBLOCK_REG
      };
      enum
      {
	PRIVATE,
	INPUT,
	OUTPUT,
	INOUT
      };
      enum
      {
	UNSIGNED,
	SIGNED
      };

      Net(int type,Expression* msb=NULL,Expression* lsb=NULL,
	  int inter=PRIVATE,Expression* sa=NULL,Expression* ea=NULL,
	  bool sign=false):
	type_(type),
	msb_(msb),
	lsb_(lsb),
	interface_(inter),
	sa_(sa),
	ea_(ea),
	sign_(sign),
	rval_(NULL)
      {}
      Net(){}
      ~Net(){}
      void setInterface(int p) { interface_=p; }
      int interface() const { return interface_; }
      void setType(int t) { type_=t; }
      int type() const { return type_; }
      bool sign() const { return sign_; }

      const string& name() const { return name_; }

      const Expression* msb() const { return msb_; }
      const Expression* lsb() const { return lsb_; }
      const Expression* sa() const { return sa_; }
      const Expression* ea() const { return ea_; }

      const Expression* rightValue() const { return rval_; }
      
      bool isArray() const
      {
	if( (sa_!=NULL)&&(ea_!=NULL) )
	  return true;
	else
	  return false;
      }

      unsigned int depth() const
      {
	if( (sa_!=NULL)&&(ea_!=NULL) )
	  return abs(ea_->calcConstant()-sa_->calcConstant())+1;
	else
	  return 0;
      }
      unsigned int width() const
      {
	if( (msb_!=NULL)&&(lsb_!=NULL) )
	  return abs(msb_->calcConstant()-lsb_->calcConstant())+1;
	else
	  if( (type_==INTEGER)||(type_==PARAMETER) )
	    return 32;
	  else
	    return 1;
      }
      
      signed calcConstant() const
      {
	if( rval_!=NULL )
	  return rval_->calcConstant();
	else
	  return 0;
      }


      void toXML(std::ostream& ostr,const string& name,int indent=0) const;
      void toVerilog(std::ostream& ostr,const string& name,
		     int indent=0,bool namedbblock=false) const;

      void link(const map<string,Net*>& net,Module* mod,const string& scope);
      Net* clone(const string& hname) const;
      Net* clone() const;

      void callback(Callback& cb) const;
    };
    ////////////////////////////////////////////////////////////////////////
    class Statement
    {
    public:
      Statement(){}
      virtual ~Statement(){}

      virtual void toXML(std::ostream& ostr,int indent=0) const {}
      virtual void toVerilog(std::ostream& ostr,int indent=0) const {}

      virtual void link(const map<string,Net*>& net,Module* mod,const string& scope) {}
      virtual Statement* clone(const string& hname) const { return NULL; }

      virtual void chain(set<const Statement*>& ss) const {}

      virtual void callback(Callback& cb) const{}
    };
    ////////////////////////////////////////////////////////////////////////
    class Block : public Statement
    {
      string             name_;
      int                type_;
      vector<Statement*> list_;
      const Module*      module_;
    public:
      enum
      {
	SEQUENTIAL,
	PARALLEL
      };
      Block(int type,const vector<Statement*>& list):
	module_(NULL),
	type_(type),
	list_(list)
      {}
      Block(int type,const vector<Statement*>& list,
	    const char* name,const Module* mod):
	type_(type),
	list_(list),
	name_(name),
	module_(mod)
      {}
      Block(int type):
	type_(type)
      {}
      Block(){}
      ~Block();
      
      const vector<Statement*>& list() const { return list_; }

      void toXML(std::ostream& ostr,int indent=0) const;
      void toVerilog(std::ostream& ostr,int indent=0) const;

      void link(const map<string,Net*>& net,Module* mod,const string& scope);
      Statement* clone(const string& hname) const;

      void chain(set<const Statement*>& ss) const;

      void callback(Callback& cb) const;
    };
    ////////////////////////////////////////////////////////////////////////
    class Case : public Statement
    {
    public:
      ////////////////////////////////////////////////////////////////////////
      class Item : public Statement
      {
	vector<Expression*> expr_;
	Statement*          stat_;
      public:
	Item(vector<Expression*> expr,Statement* stat):
	  expr_(expr),
	  stat_(stat)
	{}
	Item(Statement* stat):
	  stat_(stat)
	{}
	Item():
	  stat_(NULL)
	{}
	~Item();

	const vector<Expression*>& expression() const { return expr_; }
	const Statement* statement() const { return stat_; }

	void toXML(std::ostream& ostr,int indent=0) const;
	void toVerilog(std::ostream& ostr,int indent=0) const;

	void link(const map<string,Net*>& net,Module* mod,const string& scope);
	Item* clone(const string& hname) const;

	void chain(set<const Statement*>& ss) const;

	void callback(Callback& cb) const;
      };
    private:
      int           type_;
      Expression*   expr_;
      vector<Item*> items_;
    public:
      enum
      {
	CASE,
	CASEX,
	CASEZ
      };

      Case(int type,Expression* ex,const vector<Item*>& it):
	type_(type),
	expr_(ex),
	items_(it)
      {}
      Case():
	expr_(NULL)
      {}
      ~Case();
      int type() const { return type_; }
      Expression* expression() const { return expr_; }
      const vector<Item*>& items() const { return items_; }

      void toXML(std::ostream& ostr,int indent=0) const;
      void toVerilog(std::ostream& ostr,int indent=0) const;

      void link(const map<string,Net*>& net,Module* mod,const string& scope);
      Statement* clone(const string& hname) const;

      void chain(set<const Statement*>& ss) const;

      void callback(Callback& cb) const;
    };
    ////////////////////////////////////////////////////////////////////////
    class Condition : public Statement
    {
      Expression* expr_;
      Statement*  true_;
      Statement*  false_;
    public:
      Condition(Expression* ex,Statement* t,Statement* f=NULL):
	expr_(ex),
	true_(t),
	false_(f)
      {}
      Condition():
	expr_(NULL),
	true_(NULL),
	false_(NULL)
      {}
      ~Condition();
      const Expression* expression() const { return expr_; }
      const Statement*  trueStatement() const { return true_; }
      const Statement*  falseStatement() const { return false_; }

      void toXML(std::ostream& ostr,int indent=0) const;
      void toVerilog(std::ostream& ostr,int indent=0) const;

      void link(const map<string,Net*>& net,Module* mod,const string& scope);
      Statement* clone(const string& hname) const;

      void chain(set<const Statement*>& ss) const;

      void callback(Callback& cb) const;
    };
    ////////////////////////////////////////////////////////////////////////
    class EventStatement : public Statement
    {
      vector<Event*> event_;
      Statement*     stat_;
    public:
      EventStatement(const vector<Event*>& ee):
	event_(ee),
	stat_(NULL)
      {}
      EventStatement(Event*ee):
	event_(1),
	stat_(NULL)
      {
	event_[0] = ee;
      }
      EventStatement():
	stat_(NULL)
      {}
      ~EventStatement();
      const vector<Event*>& event() const { return event_; }
      const Statement* statement() const { return stat_; }
      void setStatement(Statement* stat){ stat_ =stat; }

      void toXML(std::ostream& ostr,int indent=0) const;
      void toVerilog(std::ostream& ostr,int indent=0) const;

      void link(const map<string,Net*>& net,Module* mod,const string& scope);
      Statement* clone(const string& hname) const;

      void chain(set<const Statement*>& ss) const;

      bool isEdge() const;
      bool isLevel() const;
      bool isStorage() const;

      void callback(Callback& cb) const;
    };
    ////////////////////////////////////////////////////////////////////////
    class Assign : public Statement
    {
      int         type_;
      Expression* lval_;
      Expression* rval_;
    public:
      enum
      {
	BLOCKING,
	NONBLOCKING
      };
      Assign(int type,Expression* lval,Expression* rval):
	type_(type),
	lval_(lval),
	rval_(rval)
      {}
      Assign():
	lval_(NULL),
	rval_(NULL)
      {}
      ~Assign();
      int type() const { return type_; }
      const Expression* leftValue()  const { return lval_; }
      const Expression* rightValue() const { return rval_; }

      void setLeftValue(Expression* e) { lval_=e; }
      void setRightValue(Expression* e) { rval_=e; }

      void toXML(std::ostream& ostr,int indent=0) const;
      void toVerilog(std::ostream& ostr,int indent=0) const;

      void link(const map<string,Net*>& net,Module* mod,const string& scope);
      Statement* clone(const string& hname) const;
      bool isSimple() const;
      bool isSimpleLeft() const;
      bool isSimpleRight() const;

      void chain(set<const Statement*>& ss) const;

      void callback(Callback& cb) const;
    };
    ////////////////////////////////////////////////////////////////////////
    class For : public Statement
    {
      Identifier* ita_;
      Expression* begin_;
      Expression* cond_;
      Expression* reach_;
      Statement*  stat_;
    public:
      For(Identifier* i1,Expression* e1,
	  Expression* e2,
	  Identifier* i2,Expression* e3,
	  Statement* s);
      For():
	ita_(NULL),
	begin_(NULL),
	cond_(NULL),
	reach_(NULL),
	stat_(NULL)
      {}
      ~For();
      const Identifier* iterat() const { return ita_; }
      const Expression* begin() const { return begin_; }
      const Expression* condition() const { return cond_; }
      const Expression* reach() const { return reach_; }
      const Statement*  statement() const { return stat_; }

      void toXML(std::ostream& ostr,int indent=0) const;
      void toVerilog(std::ostream& ostr,int indent=0) const;

      void link(const map<string,Net*>& net,Module* mod,const string& scope);
      Statement* clone(const string& hname) const;

      void chain(set<const Statement*>& ss) const;

      void callback(Callback& cb) const;
    };
    ////////////////////////////////////////////////////////////////////////
    class CallTask : public Statement
    {
      string              name_;
      vector<Expression*> args_;
    public:
      CallTask(const char* name,const vector<Expression*> &args):
	name_(name),
	args_(args)
      {}
      CallTask()
      {}
      ~CallTask(){}
      const string name() const { return name_; }
      const vector<Expression*>& arguments() const { return args_; }

      void toXML(std::ostream& ostr,int indent=0) const;
      void toVerilog(std::ostream& ostr,int indent=0) const;

      void link(const map<string,Net*>& net,Module* mod,const string& scope);
      Statement* clone(const string& hname) const;

      void chain(set<const Statement*>& ss) const;

      void callback(Callback& cb) const;
    };
    ////////////////////////////////////////////////////////////////////////
    class Function
    {
      vector<string>   port_;
      map<string,Net*> net_;
      Statement*       stat_;
    public:
      Function():
	stat_(NULL)
      {}
      ~Function();
      const vector<string>&   port()      const { return port_; }
      const map<string,Net*>& net()       const { return net_; }
      const Statement*        statement() const { return stat_; }

      void addNet(const char* name,Net* net);
      void setStatement(Statement* stat) { stat_=stat; }

      void toXML(std::ostream& ostr,int indent=0) const;
      void toVerilog(std::ostream& ostr,int indent=0) const;

      void link(Module* mod);
      Function* clone(const string& hname) const;

      void callback(Callback& cb) const;
    };
    ////////////////////////////////////////////////////////////////////////
    class Process
    {
      int        type_;
      Statement* stat_;
      string     name_;

      set<const Net*> eventChain_;
      set<const Net*> leftChain_;
      set<const Net*> rightChain_;

      set<const Net*> nbLeftChain_;
      set<const Net*> nbRightChain_;

      set<const Net*> bLeftChain_;
      set<const Net*> bRightChain_;

      set<const Statement*> statChain_;
    public:
      enum
      {
	INITIAL,
	TASK,
	ALWAYS,
	ASSIGN,
	PARAMETER
      };
      Process(int type,Statement* stat=NULL):
	type_(type),
	stat_(stat)
      {}
      Process():
	stat_(NULL)
      {}
      ~Process();
      int type() const { return type_; }
      const Statement* statement() const { return stat_; }

      void toXML(std::ostream& ostr,int indent=0) const;
      void toVerilog(std::ostream& ostr,int indent=0) const;

      void link(Module* mod);
      Process* clone(const string& hname) const;

      bool isEdge() const;
      bool isLevel() const;
      bool isStorage() const;
      const Statement* queryStatement(int type,const Net* src) const;

      const set<const Net*>& eventChain() const { return eventChain_; }
      const set<const Net*>& leftChain()  const { return leftChain_; }
      const set<const Net*>& rightChain() const { return rightChain_; }

      const set<const Net*>& nbLeftChain()  const { return nbLeftChain_; }
      const set<const Net*>& nbRightChain() const { return nbRightChain_; }
      const set<const Net*>& bLeftChain()  const { return bLeftChain_; }
      const set<const Net*>& bRightChain() const { return bRightChain_; }

      void callback(Callback& cb) const;
    };
    ////////////////////////////////////////////////////////////////////////    
    class Gate
    {
    public:
      enum
      {
	AND,
	NAND,
	OR,
	NOR,
	XOR,
	XNOR,
	BUF,
	BUFIF0,
	BUFIF1,
	NOT,
	NOTIF0,
	NOTIF1,
	PULLDOWN,
	PULLUP,
	NMOS,
	RNMOS,
	PMOS,
	RPMOS,
	CMOS,
	RCMOS,
	TRAN,
	RTRAN, 
	TRANIF0,
	TRANIF1,
	RTRANIF0,
	RTRANIF1
      };

      int                 type_;
      vector<Expression*> pin_;
    public:
      Gate(int t,const vector<Expression*>& pin):
	type_(t),
	pin_(pin)
      {}
      Gate(){}
      ~Gate();

      void callback(Callback& cb) const;
    };
    ////////////////////////////////////////////////////////////////////////    
    class Instance
    {
      ////////////////////////////////////////////////////////////////////////    
    public:
      class Port
      {
	string      ref_;
	Expression* con_;
	Net*        net_;
      public:
	Port(const char* ref,Expression* con):
	  ref_(ref),
	  con_(con),
	  net_(NULL)
	{}
	Port():
	  con_(NULL),
	  net_(NULL)
	{}
	~Port();
	const string&     reference() const { return ref_; }
	const Expression* connect() const { return con_; }

	void toXML( std::ostream& ostr,int indent=0 ) const;
	void toVerilog( std::ostream& ostr,int indent=0 ) const;

	void link(const map<string,Net*>& net,Module* mod,const string& scope,Module* rmod,int idx);
	const Net* net() const { return net_; }
	Port* clone(const string& hname) const;
	void ungroup(Module* mod,const string& cname,const string& sname);

	void callback(Callback& cb) const;
      };

      string        type_;
      vector<Port*> port_;
      Module*       module_;

      multimap<string,Expression*> params_;
    public:
      Instance(const char* t):
	type_(t),
	module_(NULL)
      {}
      Instance():
	module_(NULL)
      {}
      ~Instance();
      const string&        type() const { return type_; }
      const vector<Port*>& port() const { return port_; }
      const Module* module() const { return module_; }

      void setParameters(const multimap<string,Expression*>* p)
      {
	params_.insert(p->begin(),p->end());
      }

      void setType(const char* type){ type_ =type; }
      void addPort(Port* p);

      void toXML( std::ostream& ostr,const string& name,int indent=0 ) const;
      void toVerilog( std::ostream& ostr,const string& name,int indent=0 ) const;

      void link(Verilog* veri,const map<string,Net*>& net,
		const string& scope,Module* mod);
      Instance* clone(const string& hname) const;
      void ungroup(Module* mod,const string& cname,const string& sname);

      void callback(Callback& cb) const;
    };
    ////////////////////////////////////////////////////////////////////////
    class Module
    {
    private:
      //      Verilog&              source_;
      
      string                name_;

      vector<string>        port_;
      map<string,Net*>      net_;
      map<string,Function*> function_;
      map<string,Instance*> instance_;
      vector<Process*>      process_;

      map<string,Expression*> defparams_;
    public:
      Module(){}
      ~Module();
      //      const Verilog&               source()   const { return source_; }
      const vector<string>&        port()     const { return port_; }
      const map<string,Net*>&      net()      const { return net_; }
      const map<string,Function*>& function() const { return function_; }
      const map<string,Instance*>& instance() const { return instance_; }
      const vector<Process*>&      process()  const { return process_; }
      const string&                name()     const { return name_; }

      const map<string,Expression*>& defparam() const { return defparams_; }


      map<string,Instance*>& instance() { return instance_; }
      Instance* newInstance(const char* name);
      Function* newFunction(const char* name);
      Net*      newNet(const char* name,
		       int type,
		       Expression* msb=NULL,Expression* lsb=NULL,
		       int inter=Net::PRIVATE,
		       Expression* sa=NULL,Expression* ea=NULL,
		       bool sign=false);

      void addPort(const char* name);
      void addNet(const char* name,Net* net);
      void addAssign(Expression* l,Expression* r);
      void addParameter(Expression* l,Expression* r);
      void addProcess(Process* proc);
      void addFunction(const char* name,Function* func);
      void addInstance(const char* name,Instance* inst);

      void addDefparam(map<string,Expression*>* p)
      {
	defparams_.insert(p->begin(),p->end());
      }
      void addDefparam(string n,Expression* e)
      {
	defparams_.insert( pair<string,Expression*>(n,e) );
      }
      

      void toXML( std::ostream& ostr,const string& name,int indent=0 ) const;
      void toVerilog( std::ostream& ostr,const string& name,int indent=0 ) const;

      void link(Verilog* veri);
      void ungroup(Module* mod,const string& hname,const multimap<string,Expression*>& param);

      void link();
      void ungroup();

      const Net* findNet(const char* name) const { map<string,Net*>::const_iterator i;i=net_.find(string(name));if( i!=net_.end() ) return i->second; else return NULL; }

      //
      const char* findName(const Net* net) const
      {
	map<string,Net*>::const_iterator i;
	for( i=net_.begin();i!=net_.end();++i )
	  if( i->second==net )
	    return i->first.c_str();
	return NULL;
      }
      //

      void callback(Callback& cb) const;
    };
    ////////////////////////////////////////////////////////////////////////
    map<string,Module*> module_;
    bool                debug_;
    //    map<string,Number*> constant_;
    
    bool                dec_tpd_;
  public:
    Verilog(bool debug=false):
      debug_(debug),
      dec_tpd_(false)
    {}
    virtual ~Verilog();

    bool decTPD() const { return dec_tpd_; }
    void setDecTPD(bool flag) { dec_tpd_ =flag; }

    bool debug() const { return debug_; }

    int parse(const char* filename);
    int parse(FILE* fp);

    const map<string,Module*>& module() const { return module_; }
    Module* addModule(const char* name);

    void toXML(std::ostream& ostr,int indent=0) const;
    virtual void toVerilog(std::ostream& ostr,int indent=0) const;

    void link();
    void ungroup(Module* top);
    Module* findModule(const char* name){ map<string,Module*>::const_iterator i;i=module_.find(string(name));if( i!=module_.end() ) return i->second; else return NULL; }
    //
    const char* findName(const Module* mod) const
    {
      map<string,Module*>::const_iterator i;
      for( i=module_.begin();i!=module_.end();++i )
	if( i->second==mod )
	  return i->first.c_str();
      return NULL;
    }
    //

    void callback(Callback& cb) const;

    ////////////////////////////////////////////////////////////////////////
    class Callback
    {
    public:
      Callback(){}
      virtual ~Callback(){}

      virtual void trap(const Case::Item* self)
      {
	if( self==NULL )
	  return;

	vector<Expression*>::const_iterator i;
	for( i=self->expression().begin();i!=self->expression().end();++i )
	  (*i)->callback( *this );
	self->statement()->callback( *this );
      }
      virtual void trap(const Instance::Port* self)
      {
	if( self==NULL )
	  return;

	self->callback( *this );
      }
      virtual void trap(const Assign* self)
      {
	if( self==NULL )
	  return;

	self->rightValue()->callback( *this );
	self->leftValue()->callback( *this );
      }
      virtual void trap(const Binary* self)
      {
	if( self==NULL )
	  return;

	self->left()->callback( *this );
	self->right()->callback( *this );
      }
      virtual void trap(const Block* self)
      {
	if( self==NULL )
	  return;

	vector<Statement*>::const_iterator i;
	for( i=self->list().begin();i!=self->list().end();++i )
	  (*i)->callback( *this );
      }
      virtual void trap(const CallFunction* self)
      {
	if( self==NULL )
	  return;

	vector<Expression*>::const_iterator i;
	for( i=self->parameter().begin();i!=self->parameter().end();++i )
	  (*i)->callback( *this );
      }
      virtual void trap(const Case* self)
      {
	if( self==NULL )
	  return;

	vector<Case::Item*>::const_iterator i;
	for( i=self->items().begin();i!=self->items().end();++i )
	  {
	    if( !(*i)->expression().empty() )
	      {
		vector<Expression*>::const_iterator ii;
		for( ii=(*i)->expression().begin();ii!=(*i)->expression().end();++ii )
		  (*ii)->callback( *this );
	      }
	    (*i)->statement()->callback( *this );
	  }
      }
      virtual void trap(const Concat* self)
      {
	if( self==NULL )
	  return;

	vector<Expression*>::const_reverse_iterator i;
	for( i=self->list().rbegin();i!=self->list().rend();++i )
	  (*i)->callback( *this );
      }
      virtual void trap(const Condition* self)
      {
	if( self==NULL )
	  return;

	self->expression()->callback( *this );
	self->trueStatement()->callback( *this );
	if( self->falseStatement()!=NULL )
	  self->falseStatement()->callback( *this );
      }
      virtual void trap(const Event* self)
      {
	if( self==NULL )
	  return;

	self->expression()->callback( *this );
      }
      virtual void trap(const EventStatement* self)
      {
	if( self==NULL )
	  return;

	vector<Event*>::const_reverse_iterator i;
	for( i=self->event().rbegin();i!=self->event().rend();++i )
	  (*i)->callback( *this );
	self->statement()->callback( *this );
      }
      virtual void trap(const For* self)
      {
	if( self==NULL )
	  return;

	//	self->iterat()->callback( *this );
	//	self->begin()->callback( *this );
	//	self->condition()->callback( *this );
	//	self->reach()->callback( *this );

	self->statement()->callback( *this );
      }
      virtual void trap(const CallTask* self)
      {
	if( self==NULL )
	  return;

      }
      virtual void trap(const Function* self)
      {
	if( self==NULL )
	  return;

	self->statement()->callback( *this );
      }
      virtual void trap(const Gate* self){}
      virtual void trap(const Identifier* self)
      {
	if( self==NULL )
	  return;

	if( self->msb()!=NULL )
	  self->msb()->callback( *this );
	if( self->lsb()!=NULL )
	  self->lsb()->callback( *this );
	if( self->idx()!=NULL )
	  self->idx()->callback( *this );
	if( self->net()!=NULL )
	  self->net()->callback( *this );
      }
      virtual void trap(const Instance* self)
      {
	if( self==NULL )
	  return;

	vector<Instance::Port*>::const_iterator i;
	for( i=self->port().begin();i!=self->port().end();++i )
	  (*i)->callback( *this );
      }
      virtual void trap(const Module* self)
      {
	if( self==NULL )
	  return;
	{
	  map<string,Net*>::const_iterator i;
	  for( i=self->net().begin();i!=self->net().end();++i )
	    i->second->callback( *this );
	}
	{
	  map<string,Function*>::const_iterator i;
	  for( i=self->function().begin();i!=self->function().end();++i )
	    i->second->callback( *this );
	}
	{
	  map<string,Instance*>::const_iterator i;
	  for( i=self->instance().begin();i!=self->instance().end();++i )
	    i->second->callback( *this );
	}
	{
	  vector<Process*>::const_iterator i;
	  for( i=self->process().begin();i!=self->process().end();++i )
	    (*i)->callback( *this );
	}
      }
      virtual void trap(const Net* self)
      {
	if( self==NULL )
	  return;
	
	if( self->msb()!=NULL )
	  self->msb()->callback( *this );
	if( self->lsb()!=NULL )
	  self->lsb()->callback( *this );
	if( self->sa()!=NULL )
	  self->sa()->callback( *this );
	if( self->ea()!=NULL )
	  self->ea()->callback( *this );
      }
      virtual void trap(const String* self){}
      virtual void trap(const Number* self){}
      virtual void trap(const Process* self)
      {
	if( self==NULL )
	  return;

	self->statement()->callback( *this );
	{
	  set<const Net*>::const_iterator i;
	  for( i=self->eventChain().begin();i!=self->eventChain().end();++i )
	    (*i)->callback( *this );
	}	
      }
      virtual void trap(const Ternary* self)
      {
	if( self==NULL )
	  return;

	if( self->condition()!=NULL )
	  self->condition()->callback( *this );
	if( self->trueValue()!=NULL )
	  self->trueValue()->callback( *this );
	if( self->falseValue()!=NULL )
	  self->falseValue()->callback( *this );
      }
      virtual void trap(const Unary* self)
      {
	if( self==NULL )
	  return;

	if( self->value()!=NULL )
	  self->value()->callback( *this );
      }
      virtual void trap(const Verilog* self)
      {
	if( self==NULL )
	  return;

	map<string,Module*>::const_iterator i;
	for( i=self->module().begin();i!=self->module().end();++i )
	  i->second->callback( *this );	
      }
    };
    ////////////////////////////////////////////////////////////////////////
    class LeftNetChainCB : public Callback
    {
      set<const Net*>& chain_;
    public:
      LeftNetChainCB(set<const Net*>& chain):
	chain_(chain)
      {}
      ~LeftNetChainCB(){}

      void trap(const Case::Item* self);
      void trap(const Assign* self);
      void trap(const Block* self);
      void trap(const Case* self);
      void trap(const Concat* self);
      void trap(const Condition* self);
      void trap(const EventStatement* self);
      void trap(const Identifier* self);
      void trap(const Net* self);
      void trap(const Process* self);
    };
    ////////////////////////////////////////////////////////////////////////
    class RightNetChainCB : public Callback
    {
      set<const Net*>& chain_;
      bool             left_;
    public:
      RightNetChainCB(set<const Net*>& chain):
	chain_(chain),
	left_(false)
      {}
      ~RightNetChainCB(){}

      void trap(const Case::Item* self);
      void trap(const Assign* self);
      void trap(const Block* self);
      void trap(const Case* self);
      void trap(const Condition* self);
      void trap(const EventStatement* self);
      void trap(const Net* self);
      void trap(const Process* self);

      void trap(const Binary* self);
      void trap(const CallFunction* self);
      void trap(const Concat* self);
      void trap(const Ternary* self);
      void trap(const Unary* self);
      void trap(const Identifier* self);      
    };
    ////////////////////////////////////////////////////////////////////////
    class EventNetChainCB : public Callback
    {
      set<const Net*>& chain_;
    public:
      EventNetChainCB(set<const Net*>& chain):
	chain_(chain)
      {}
      ~EventNetChainCB(){}

      void trap(const Process* self);
      void trap(const EventStatement* self);
      void trap(const Event* self);
      void trap(const Identifier* self);
      void trap(const Net* self);
    };
    ////////////////////////////////////////////////////////////////////////
    class NetChainCB : public Callback
    {
      set<const Net*>& nbLeftChain_;
      set<const Net*>& nbRightChain_;
      set<const Net*>& bLeftChain_;
      set<const Net*>& bRightChain_;

      bool             left_;
      bool             blocking_;

    public:
      NetChainCB(set<const Net*>& nbLeftChain,
		 set<const Net*>& nbRightChain,
		 set<const Net*>& bLeftChain,
		 set<const Net*>& bRightChain):
	nbLeftChain_(nbLeftChain),
	nbRightChain_(nbRightChain),
	bLeftChain_(bLeftChain),
	bRightChain_(bRightChain),
	left_(false),
	blocking_(false)
      {}
      ~NetChainCB(){}

      void trap(const Case::Item* self);
      void trap(const Assign* self);
      void trap(const Block* self);
      void trap(const Case* self);
      void trap(const Concat* self);
      void trap(const Condition* self);
      void trap(const EventStatement* self);
      void trap(const Identifier* self);
      void trap(const Net* self);
      void trap(const Process* self);

      // only right net 
      void trap(const Binary* self);
      void trap(const CallFunction* self);
      void trap(const Ternary* self);
      void trap(const Unary* self);
    };
    ////////////////////////////////////////////////////////////////////////
  };
}

#endif




#line 2 "_lexor.cc"

#line 4 "_lexor.cc"

#define  YY_INT_ALIGNED short int

/* A lexical scanner generated by flex */

#define yy_create_buffer verilog__create_buffer
#define yy_delete_buffer verilog__delete_buffer
#define yy_flex_debug verilog__flex_debug
#define yy_init_buffer verilog__init_buffer
#define yy_flush_buffer verilog__flush_buffer
#define yy_load_buffer_state verilog__load_buffer_state
#define yy_switch_to_buffer verilog__switch_to_buffer
#define yyin verilog_in
#define yyleng verilog_leng
#define yylex verilog_lex
#define yylineno verilog_lineno
#define yyout verilog_out
#define yyrestart verilog_restart
#define yytext verilog_text
#define yywrap verilog_wrap
#define yyalloc verilog_alloc
#define yyrealloc verilog_realloc
#define yyfree verilog_free

#define FLEX_SCANNER
#define YY_FLEX_MAJOR_VERSION 2
#define YY_FLEX_MINOR_VERSION 6
#define YY_FLEX_SUBMINOR_VERSION 0
#if YY_FLEX_SUBMINOR_VERSION > 0
#define FLEX_BETA
#endif

/* First, we deal with  platform-specific or compiler-specific issues. */

/* begin standard C headers. */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

/* end standard C headers. */

/* flex integer type definitions */

#ifndef FLEXINT_H
#define FLEXINT_H

/* C99 systems have <inttypes.h>. Non-C99 systems may or may not. */

#if defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L

/* C99 says to define __STDC_LIMIT_MACROS before including stdint.h,
 * if you want the limit (max/min) macros for int types. 
 */
#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS 1
#endif

#include <inttypes.h>
typedef int8_t flex_int8_t;
typedef uint8_t flex_uint8_t;
typedef int16_t flex_int16_t;
typedef uint16_t flex_uint16_t;
typedef int32_t flex_int32_t;
typedef uint32_t flex_uint32_t;
#else
typedef signed char flex_int8_t;
typedef short int flex_int16_t;
typedef int flex_int32_t;
typedef unsigned char flex_uint8_t; 
typedef unsigned short int flex_uint16_t;
typedef unsigned int flex_uint32_t;

/* Limits of integral types. */
#ifndef INT8_MIN
#define INT8_MIN               (-128)
#endif
#ifndef INT16_MIN
#define INT16_MIN              (-32767-1)
#endif
#ifndef INT32_MIN
#define INT32_MIN              (-2147483647-1)
#endif
#ifndef INT8_MAX
#define INT8_MAX               (127)
#endif
#ifndef INT16_MAX
#define INT16_MAX              (32767)
#endif
#ifndef INT32_MAX
#define INT32_MAX              (2147483647)
#endif
#ifndef UINT8_MAX
#define UINT8_MAX              (255U)
#endif
#ifndef UINT16_MAX
#define UINT16_MAX             (65535U)
#endif
#ifndef UINT32_MAX
#define UINT32_MAX             (4294967295U)
#endif

#endif /* ! C99 */

#endif /* ! FLEXINT_H */

#ifdef __cplusplus

/* The "const" storage-class-modifier is valid. */
#define YY_USE_CONST

#else	/* ! __cplusplus */

/* C99 requires __STDC__ to be defined as 1. */
#if defined (__STDC__)

#define YY_USE_CONST

#endif	/* defined (__STDC__) */
#endif	/* ! __cplusplus */

#ifdef YY_USE_CONST
#define yyconst const
#else
#define yyconst
#endif

/* Returned upon end-of-file. */
#define YY_NULL 0

/* Promotes a possibly negative, possibly signed char to an unsigned
 * integer for use as an array index.  If the signed char is negative,
 * we want to instead treat it as an 8-bit unsigned char, hence the
 * double cast.
 */
#define YY_SC_TO_UI(c) ((unsigned int) (unsigned char) c)

/* Enter a start condition.  This macro really ought to take a parameter,
 * but we do it the disgusting crufty way forced on us by the ()-less
 * definition of BEGIN.
 */
#define BEGIN (yy_start) = 1 + 2 *

/* Translate the current start state into a value that can be later handed
 * to BEGIN to return to the state.  The YYSTATE alias is for lex
 * compatibility.
 */
#define YY_START (((yy_start) - 1) / 2)
#define YYSTATE YY_START

/* Action number for EOF rule of a given start state. */
#define YY_STATE_EOF(state) (YY_END_OF_BUFFER + state + 1)

/* Special action meaning "start processing a new file". */
#define YY_NEW_FILE verilog_restart(verilog_in  )

#define YY_END_OF_BUFFER_CHAR 0

/* Size of default input buffer. */
#ifndef YY_BUF_SIZE
#ifdef __ia64__
/* On IA-64, the buffer size is 16k, not 8k.
 * Moreover, YY_BUF_SIZE is 2*YY_READ_BUF_SIZE in the general case.
 * Ditto for the __ia64__ case accordingly.
 */
#define YY_BUF_SIZE 32768
#else
#define YY_BUF_SIZE 16384
#endif /* __ia64__ */
#endif

/* The state buf must be large enough to hold one state per character in the main buffer.
 */
#define YY_STATE_BUF_SIZE   ((YY_BUF_SIZE + 2) * sizeof(yy_state_type))

#ifndef YY_TYPEDEF_YY_BUFFER_STATE
#define YY_TYPEDEF_YY_BUFFER_STATE
typedef struct yy_buffer_state *YY_BUFFER_STATE;
#endif

#ifndef YY_TYPEDEF_YY_SIZE_T
#define YY_TYPEDEF_YY_SIZE_T
typedef size_t yy_size_t;
#endif

extern yy_size_t verilog_leng;

extern FILE *verilog_in, *verilog_out;

#define EOB_ACT_CONTINUE_SCAN 0
#define EOB_ACT_END_OF_FILE 1
#define EOB_ACT_LAST_MATCH 2

    #define YY_LESS_LINENO(n)
    #define YY_LINENO_REWIND_TO(ptr)
    
/* Return all but the first "n" matched characters back to the input stream. */
#define yyless(n) \
	do \
		{ \
		/* Undo effects of setting up verilog_text. */ \
        int yyless_macro_arg = (n); \
        YY_LESS_LINENO(yyless_macro_arg);\
		*yy_cp = (yy_hold_char); \
		YY_RESTORE_YY_MORE_OFFSET \
		(yy_c_buf_p) = yy_cp = yy_bp + yyless_macro_arg - YY_MORE_ADJ; \
		YY_DO_BEFORE_ACTION; /* set up verilog_text again */ \
		} \
	while ( 0 )

#define unput(c) yyunput( c, (yytext_ptr)  )

#ifndef YY_STRUCT_YY_BUFFER_STATE
#define YY_STRUCT_YY_BUFFER_STATE
struct yy_buffer_state
	{
	FILE *yy_input_file;

	char *yy_ch_buf;		/* input buffer */
	char *yy_buf_pos;		/* current position in input buffer */

	/* Size of input buffer in bytes, not including room for EOB
	 * characters.
	 */
	yy_size_t yy_buf_size;

	/* Number of characters read into yy_ch_buf, not including EOB
	 * characters.
	 */
	int yy_n_chars;

	/* Whether we "own" the buffer - i.e., we know we created it,
	 * and can realloc() it to grow it, and should free() it to
	 * delete it.
	 */
	int yy_is_our_buffer;

	/* Whether this is an "interactive" input source; if so, and
	 * if we're using stdio for input, then we want to use getc()
	 * instead of fread(), to make sure we stop fetching input after
	 * each newline.
	 */
	int yy_is_interactive;

	/* Whether we're considered to be at the beginning of a line.
	 * If so, '^' rules will be active on the next match, otherwise
	 * not.
	 */
	int yy_at_bol;

    int yy_bs_lineno; /**< The line count. */
    int yy_bs_column; /**< The column count. */
    
	/* Whether to try to fill the input buffer when we reach the
	 * end of it.
	 */
	int yy_fill_buffer;

	int yy_buffer_status;

#define YY_BUFFER_NEW 0
#define YY_BUFFER_NORMAL 1
	/* When an EOF's been seen but there's still some text to process
	 * then we mark the buffer as YY_EOF_PENDING, to indicate that we
	 * shouldn't try reading from the input source any more.  We might
	 * still have a bunch of tokens to match, though, because of
	 * possible backing-up.
	 *
	 * When we actually see the EOF, we change the status to "new"
	 * (via verilog_restart()), so that the user can continue scanning by
	 * just pointing verilog_in at a new input file.
	 */
#define YY_BUFFER_EOF_PENDING 2

	};
#endif /* !YY_STRUCT_YY_BUFFER_STATE */

/* Stack of input buffers. */
static size_t yy_buffer_stack_top = 0; /**< index of top of stack. */
static size_t yy_buffer_stack_max = 0; /**< capacity of stack. */
static YY_BUFFER_STATE * yy_buffer_stack = 0; /**< Stack as an array. */

/* We provide macros for accessing buffer states in case in the
 * future we want to put the buffer states in a more general
 * "scanner state".
 *
 * Returns the top of the stack, or NULL.
 */
#define YY_CURRENT_BUFFER ( (yy_buffer_stack) \
                          ? (yy_buffer_stack)[(yy_buffer_stack_top)] \
                          : NULL)

/* Same as previous macro, but useful when we know that the buffer stack is not
 * NULL or when we need an lvalue. For internal use only.
 */
#define YY_CURRENT_BUFFER_LVALUE (yy_buffer_stack)[(yy_buffer_stack_top)]

/* yy_hold_char holds the character lost when verilog_text is formed. */
static char yy_hold_char;
static int yy_n_chars;		/* number of characters read into yy_ch_buf */
yy_size_t verilog_leng;

/* Points to current character in buffer. */
static char *yy_c_buf_p = (char *) 0;
static int yy_init = 0;		/* whether we need to initialize */
static int yy_start = 0;	/* start state number */

/* Flag which is used to allow verilog_wrap()'s to do buffer switches
 * instead of setting up a fresh verilog_in.  A bit of a hack ...
 */
static int yy_did_buffer_switch_on_eof;

void verilog_restart (FILE *input_file  );
void verilog__switch_to_buffer (YY_BUFFER_STATE new_buffer  );
YY_BUFFER_STATE verilog__create_buffer (FILE *file,int size  );
void verilog__delete_buffer (YY_BUFFER_STATE b  );
void verilog__flush_buffer (YY_BUFFER_STATE b  );
void verilog_push_buffer_state (YY_BUFFER_STATE new_buffer  );
void verilog_pop_buffer_state (void );

static void verilog_ensure_buffer_stack (void );
static void verilog__load_buffer_state (void );
static void verilog__init_buffer (YY_BUFFER_STATE b,FILE *file  );

#define YY_FLUSH_BUFFER verilog__flush_buffer(YY_CURRENT_BUFFER )

YY_BUFFER_STATE verilog__scan_buffer (char *base,yy_size_t size  );
YY_BUFFER_STATE verilog__scan_string (yyconst char *yy_str  );
YY_BUFFER_STATE verilog__scan_bytes (yyconst char *bytes,yy_size_t len  );

void *verilog_alloc (yy_size_t  );
void *verilog_realloc (void *,yy_size_t  );
void verilog_free (void *  );

#define yy_new_buffer verilog__create_buffer

#define yy_set_interactive(is_interactive) \
	{ \
	if ( ! YY_CURRENT_BUFFER ){ \
        verilog_ensure_buffer_stack (); \
		YY_CURRENT_BUFFER_LVALUE =    \
            verilog__create_buffer(verilog_in,YY_BUF_SIZE ); \
	} \
	YY_CURRENT_BUFFER_LVALUE->yy_is_interactive = is_interactive; \
	}

#define yy_set_bol(at_bol) \
	{ \
	if ( ! YY_CURRENT_BUFFER ){\
        verilog_ensure_buffer_stack (); \
		YY_CURRENT_BUFFER_LVALUE =    \
            verilog__create_buffer(verilog_in,YY_BUF_SIZE ); \
	} \
	YY_CURRENT_BUFFER_LVALUE->yy_at_bol = at_bol; \
	}

#define YY_AT_BOL() (YY_CURRENT_BUFFER_LVALUE->yy_at_bol)

/* Begin user sect3 */

typedef unsigned char YY_CHAR;

FILE *verilog_in = (FILE *) 0, *verilog_out = (FILE *) 0;

typedef int yy_state_type;

extern int verilog_lineno;

int verilog_lineno = 1;

extern char *verilog_text;
#ifdef yytext_ptr
#undef yytext_ptr
#endif
#define yytext_ptr verilog_text

static yy_state_type yy_get_previous_state (void );
static yy_state_type yy_try_NUL_trans (yy_state_type current_state  );
static int yy_get_next_buffer (void );
#if defined(__GNUC__) && __GNUC__ >= 3
__attribute__((__noreturn__))
#endif
static void yy_fatal_error (yyconst char msg[]  );

/* Done after the current pattern has been matched and before the
 * corresponding action - sets up verilog_text.
 */
#define YY_DO_BEFORE_ACTION \
	(yytext_ptr) = yy_bp; \
	(yytext_ptr) -= (yy_more_len); \
	verilog_leng = (size_t) (yy_cp - (yytext_ptr)); \
	(yy_hold_char) = *yy_cp; \
	*yy_cp = '\0'; \
	(yy_c_buf_p) = yy_cp;

#define YY_NUM_RULES 250
#define YY_END_OF_BUFFER 251
/* This struct is not used in this scanner,
   but its presence is necessary. */
struct yy_trans_info
	{
	flex_int32_t yy_verify;
	flex_int32_t yy_nxt;
	};
static yyconst flex_int16_t yy_accept[1185] =
    {   0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,  251,  249,    2,    3,  184,  185,
      184,  249,  184,  249,  184,  184,  184,  184,  184,  184,
      219,  184,  184,  184,  206,  249,  184,  249,  206,  206,
      206,  206,  206,  206,  206,  206,  206,  206,  206,  206,
      206,  206,  206,  206,  206,  206,  206,  206,  206,  206,
      184,  184,  249,   12,   13,   12,    8,    9,    8,    5,
        6,  189,  187,  188,  189,  184,  184,  184,  204,  184,
      200,  201,  204,  202,  203,  200,  201,  204,  204,  202,
      203,  250,   25,  209,  209,  209,  209,  209,  209,  209,

      209,  209,  209,  209,   29,    0,    0,    0,    0,    0,
       11,   19,   23,   38,   39,   34,    0,  210,    7,    4,
        0,    0,    0,  219,    0,   15,   20,   24,   22,   21,
       16,  206,    0,  208,   32,  205,  205,  205,  205,  205,
      205,  205,  205,  205,  205,  205,  206,  206,  206,  206,
      206,  206,  206,  206,  206,  206,  206,  206,  206,  206,
      206,  206,  206,  206,  206,   78,  206,  206,  206,  206,
      206,  206,  206,  206,  206,  206,  206,  206,   98,  206,
      206,  206,  206,  206,  206,  206,  206,  206,  206,  206,
      206,  206,  206,  206,  206,  206,  206,  206,  206,  206,

      206,  206,  206,  206,  206,  206,  206,  206,  206,   28,
       33,   31,   30,  205,   14,   10,  186,    0,    0,    0,
        0,   27,  209,  209,  209,  209,  209,  209,  209,  209,
      209,  209,  209,  209,  209,  209,  209,   35,    0,  216,
        0,  215,    0,  218,    0,  217,  210,    0,    0,    0,
        0,    0,  220,    0,  221,   17,   26,   18,  207,  205,
      205,  205,  205,  205,  205,  205,  205,  205,  205,  205,
      205,  205,  205,  205,  206,   41,  206,  206,  206,   45,
      206,  206,  206,  206,  206,  206,  206,  206,  206,   58,
      206,   69,  206,  206,  206,  206,  206,  206,  206,  206,

      206,  206,  206,  206,  206,  206,  206,  206,  206,  206,
      206,  206,   94,   95,  206,  206,  206,  206,  206,  206,
      206,  206,  111,  206,  206,  206,  206,  206,  206,  206,
      206,  206,  206,  206,  206,  206,  206,  206,  134,  206,
      154,  206,  206,  206,  206,  206,  206,  148,  206,  150,
      205,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,  209,  209,  209,  209,  209,  162,  209,  164,  209,
      209,  209,  209,  209,  209,  209,  209,  209,  209,  215,
        0,  212,    0,  211,    0,  214,    0,  213,  220,    0,
      221,  207,  205,  205,  205,  205,  205,  205,  205,  205,

      205,  205,  205,  205,  205,  205,  205,  205,  205,  205,
      205,  205,  206,  206,  206,  206,  206,   48,  153,   51,
      206,  206,  206,  206,  206,   56,   57,  206,  206,  206,
      206,  206,  206,  206,  206,  206,  206,   72,  206,  206,
      206,  206,  206,  206,  206,  206,  206,  206,  206,  206,
       84,  206,  206,  206,  206,  206,  206,  206,   91,  206,
       93,  206,  206,  206,  101,  206,  206,  206,  206,  109,
      206,  206,  206,  206,  206,  206,  206,  206,  206,  206,
      206,  206,  129,  130,  131,  135,  136,  206,  206,  206,
      206,  206,  142,  143,  206,  206,  147,  149,  205,  194,

      195,  196,  197,  190,  191,  193,  192,  198,  199,  209,
      209,  209,  209,  209,  163,  209,  209,  209,  209,  171,
      209,  209,  209,  209,  209,  209,  211,    0,  220,  205,
      205,  205,  205,  205,  205,  205,  205,  205,  205,  205,
      205,  205,  205,  205,  205,  205,  205,  205,  205,  205,
      205,  205,  206,  206,  206,   44,  206,   49,   50,  206,
      206,  206,  206,  206,  206,  206,  206,  206,  206,  206,
      206,  206,   68,   70,  206,  206,  206,  206,  206,  206,
      206,  206,  206,   81,   82,  206,  206,   85,  206,  206,
      206,  206,  206,  206,  206,  206,  206,  206,  206,  206,

      104,  105,  206,  206,  108,  206,  206,  206,  114,  115,
      116,  206,  206,  121,  206,  206,  206,  206,  128,  206,
      206,  138,  206,  206,  206,  144,  145,  146,  205,  209,
      209,  209,  209,  161,  165,  166,  167,  209,  172,  173,
      209,  209,  209,  209,  209,  220,  205,  205,  205,  205,
      205,  205,  228,  228,  205,  205,  205,  205,  205,  205,
      205,  205,  205,  205,  205,  205,  205,  205,  205,  205,
      205,   40,   42,  206,   46,   47,  206,  206,  206,  151,
      206,  206,  206,  206,  206,  206,  206,  206,  206,  206,
      206,  206,  206,   75,   76,   77,   79,  206,  206,  206,

      206,  206,  206,  206,  206,  206,   89,   90,  206,   96,
       97,   99,  206,  206,  206,  206,  107,  206,  206,  113,
      206,  206,  120,  206,  206,  206,  206,  206,  137,  139,
      206,  206,    0,  209,  209,  209,  209,  209,  209,  209,
      209,   36,  209,  209,  205,  205,  205,  205,  205,  205,
      205,  230,  230,  205,  205,  205,  234,  234,  205,  205,
      205,  205,  205,  205,  205,  205,  205,  205,  205,  248,
      248,  205,  206,  206,   53,  206,   55,   59,  206,  206,
      206,  206,  206,  206,  206,   67,   71,  206,  206,  157,
      156,   80,  206,   83,  155,   86,  206,  206,   92,  206,

      102,  206,  206,  206,  112,  206,  206,  122,  206,  124,
      125,  126,  127,  132,  133,  206,  206,    0,  209,  209,
      209,  160,  209,  209,  170,  209,  209,  209,  209,  205,
      205,  205,  205,  227,  227,  205,  205,  205,  205,  205,
      205,  205,  205,  205,  205,  205,  205,  205,  205,  205,
      206,   52,   54,  206,  206,  206,  206,  206,  206,   66,
       73,   74,  152,  206,  206,  206,  206,  106,  110,  117,
      118,  119,  206,  140,  141,    0,  209,  209,  209,  209,
      209,  209,  175,  209,   37,  205,  205,  205,  205,  205,
      205,  205,  205,  235,  235,  205,  205,  205,  205,  205,

      241,  205,  205,  205,  205,  205,  205,   43,   60,  206,
      206,   63,  206,  206,  206,  206,  100,  103,  123,    0,
      209,  209,  209,  209,  209,  209,  176,  177,  205,  205,
      205,  205,  205,  205,  205,  205,  205,  205,  205,  205,
      205,  205,  205,  205,  205,  245,  245,  205,  205,  205,
      206,  206,  206,   65,   87,  206,    0,    1,  209,  159,
      209,  209,  174,  205,  205,  205,  205,  205,  205,  205,
      205,  205,  205,  205,  205,  205,  205,  205,  242,  205,
      205,  246,  246,  205,  205,   61,   62,  206,   88,    1,
      209,  168,  169,  223,  223,  205,  205,  225,  225,  205,

      205,  205,  205,  205,  205,  231,  205,  205,  205,  205,
      205,  205,  205,  205,  205,  205,   64,  158,  205,  205,
      205,  205,  205,  205,  205,  205,  205,  205,  205,  205,
      205,  205,  205,  205,  205,  205,  205,  205,  205,  205,
      205,  205,  205,  205,  205,  205,  232,  205,  236,  236,
      205,  205,  205,  205,  205,  205,  205,  205,  205,  205,
      205,  205,  205,  205,  205,  205,  229,  229,  205,  205,
      205,  205,  205,  205,  205,  205,  205,  205,  205,  205,
      205,  205,  181,  182,  183,  205,  205,  205,  205,  205,
      205,  205,  205,  205,  205,  226,  226,  205,  205,  205,

      181,  181,  181,  182,  182,  182,  183,  183,  183,  205,
      205,  205,  205,  205,  205,  244,  244,  205,  205,  205,
      205,  205,  205,  205,  205,  205,  205,  205,  243,  243,
      205,  205,  205,  178,  205,  205,  233,  233,  205,  205,
      205,  239,  239,  205,  205,  247,  247,  205,  205,  178,
      178,  178,  205,  205,  205,  238,  238,  205,  205,  205,
      205,  205,  237,  237,  205,  240,  240,  205,  205,  205,
      205,  224,  224,  205,  205,  180,  179,  180,  180,  180,
      179,  179,  179,    0
    } ;

static yyconst YY_CHAR yy_ec[256] =
    {   0,
        1,    1,    1,    1,    1,    1,    1,    2,    3,    4,
        1,    2,    2,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    5,    6,    7,    8,    9,    8,   10,   11,   12,
       13,   14,   15,    8,   16,   17,   18,   19,   20,   21,
       21,   21,   21,   21,   21,   22,   22,   23,    8,   24,
       25,   26,   27,    8,   28,   29,   28,   30,   31,   32,
       33,   34,   33,   33,   33,   33,   33,   35,   36,   35,
       33,   37,   38,   33,   33,   33,   33,   39,   33,   40,
        8,   41,    8,   42,   43,   44,   45,   46,   47,   48,

       49,   50,   51,   52,   53,   54,   55,   56,   57,   58,
       59,   60,   33,   61,   62,   63,   64,   65,   66,   67,
       68,   69,    8,   70,    8,   71,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,

        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1
    } ;

static yyconst YY_CHAR yy_meta[72] =
    {   0,
        1,    2,    3,    4,    3,    1,    1,    1,    5,    1,
        1,    1,    1,    1,    1,    1,    6,    1,    7,    7,
        7,    8,    1,    1,    1,    1,    9,   10,   10,   10,
       10,   10,   11,   11,   11,   11,   11,   11,   12,   12,
        1,    1,   12,    1,   10,   10,   10,   10,   10,   10,
       11,   11,   11,   11,   11,   11,   11,   11,   11,   11,
       11,   11,   11,   11,   11,   11,   12,   11,   12,    1,
        1
    } ;

static yyconst flex_uint16_t yy_base[1261] =
    {   0,
        0, 2400,   68,   69,   70,   71, 2439, 2438,   72,   73,
      103, 2397,    0,    0, 2440, 5954, 5954, 5954, 2414, 5954,
     5954,   41, 2428,   95, 2423,   64, 2413,   72,  104,   73,
      168,   76,   85,   95, 2418,    0, 2362,  133,  110,   87,
       99,  109,  111,   91,  128,  122,  148,  101,  167,  178,
      183,  175,  184,  186,  204,  202,  192,  166,  207,  213,
     2360,  192,  228, 5954, 5954, 2416, 5954, 5954, 2410, 5954,
     5954, 5954, 5954, 5954, 2420,  260,   67,  111,    0, 5954,
     2409, 2408, 2407, 2406, 2405,  221,  236,  248,  261,  252,
      265, 5954, 2396,    0, 2357, 2360, 2369, 2354, 2362,   93,

     2351, 2360,  281, 2341, 2384,  311,  336,  202,  205,  331,
     5954, 5954, 5954, 5954, 5954, 5954,  211,    0, 5954, 5954,
      291,  355,  324,    0,  353, 2364, 5954, 2362, 5954, 5954,
     2360, 2368,    0,    0, 5954,    0,  257, 2335, 2334,  330,
      240, 2323, 2320, 2331, 2326, 2320,  266,  292,  241,  296,
      302,  312,  319,  291,  335,  354,  320,  347,  346,  300,
      353,  349,  318,  370,  375,  378,  388,  379,  383,  392,
      395,  396,  389,  398,  394,  402,  401,  403, 2360,  404,
      407,  406,  408,  405,  413,  414,  428,  417,  423,  416,
      410,  412,  437,  432,  422,  439,  440,  444,  442,  445,

      455,  458,  459,  461,  468,  472,  473,  476,  475, 5954,
     5954, 5954, 5954, 2323, 5954, 5954, 5954,  471,  475,  489,
      477, 5954, 2312, 2310, 2310,  171, 2304, 2309, 2319,  455,
     2307, 2303,  455, 2308, 2315, 2316, 2302, 5954,  510,  500,
      541,  504,  549,  552,  561,  551,    0,  582,  577,  570,
      571,  578,  584,  619,  623, 5954, 5954, 5954, 2345,    0,
     2312, 2295, 2301,  472, 2294, 2307, 2294, 2305, 2305,  568,
     2292,  441, 2293,  484,  541, 2332,  528,  524,  531,  542,
      551,  572,  548,  594,  571,  602,  617,  618,  619,  629,
      534,  635,  576,  636,  631,  637,  646,  630,  633,  640,

      642,  654,  641,  644,  652,  643,  648,  647,  653,  658,
      661,  656, 2331,  670,  655,  681,  657,  690,  664,  660,
      694,  695, 2330,  697,  703,  705,  707,  710,  711,  712,
      713,  714,  715,  708,  716,  294,  724,  717,  718,  723,
     2324,  719,  725,  730,  726,  727,  731, 2323,  728, 2322,
     2280, 2315, 2310, 2304, 2303, 2302, 2301, 2300, 2299, 2298,
     2297, 2248, 2250, 2247, 2244, 2241,    0, 2241,    0, 2254,
     2243,  708, 2252, 2244, 2239, 2232, 2239, 2251, 2242,  765,
      787,  776,  815,  778,  751,  755,  788,  812,  827,  844,
      852, 2277, 2244, 2232, 2232,  749, 2242, 2237,  757, 2240,

     2235, 2227, 2235, 2214, 2231, 2221, 2215, 2218, 2227, 2221,
     2210, 2219,  733,  806,  748,  792,  794,  808, 2250, 2240,
      805,  821,  795,  811,  796, 2235, 2229,  824,  836,  812,
      851,  861,  863,  867,  865,  869,  870, 2228,  871,  872,
      874,  873,  879,  876,  875,  877,  880,  881,  882,  885,
     2227,  889,  887,  886,  890,  888,  884,  894, 2226,  897,
     2225,  891,  892,  896, 2224,  903,  899,  938,  898,  900,
      904,  909,  907,  908,  915,  942,  944,  945,  947,  948,
      949,  950, 2223, 2222,  951, 2221, 2220,  954,  955,  959,
      958,  960, 2219, 2218,  961,  962, 2217, 2216, 2183, 5954,

     5954, 5954, 5954, 5954, 5954, 5954, 5954, 5954, 5954, 2178,
     2167, 2184, 2183, 2164,    0, 2158, 2174, 2161, 2178,    0,
     2173, 2171,  160, 2168, 2168, 2164,  970,  963,  975, 2158,
     2164, 2164, 2147, 2152, 2141, 1019, 2159, 2157, 2145, 2147,
     2149, 2134, 2150, 2136, 2129, 2134, 2126, 2109, 2110, 2110,
     2113, 2120,  986,  957,  993, 2152, 1030, 2151, 2150, 1000,
     1006,  997,  981, 1011, 1023, 1024, 1034, 1035, 1038, 1042,
     1039, 1037, 2149, 2148, 1047, 1048, 1049, 1051, 1052, 1053,
     1056, 1057, 1058, 2147, 2146, 1040, 1059, 2145, 1060, 1062,
     1061, 1063, 1067, 1064, 1074, 1087, 1070, 1080, 1083, 1092,

     2144, 2143, 1093, 1094, 2142, 1097, 1098, 1099, 2141, 2140,
     1110, 1109, 1101, 2139, 1111, 1102, 1113, 1114, 2138, 1115,
     1118, 2137, 1120, 1119, 1122, 2136, 2135, 2134, 2144, 2100,
     2097, 2081, 2078,    0,    0,    0, 2099, 2080,    0,    0,
     2093, 2094, 2090, 2080, 2078, 1124, 2086, 2067, 2079, 2071,
     2077, 2082,    0, 1154, 1183, 2059, 1254, 2051, 2056, 1325,
     2055, 2053, 2056, 2041, 2040, 2051, 2048, 2051, 2048, 2036,
     1396, 2076, 2075, 1124, 2074, 2073, 1121, 1141, 1123, 2072,
     1125, 1131, 1152, 1134, 1156, 1158, 1159, 1186, 1136, 1162,
     1197, 1161, 1211, 2071, 2070, 2069, 2068, 1212, 1194, 1195,

     1198, 1202, 1213, 1214, 1196, 1225, 2067, 2066, 1201, 2065,
     2064, 2063, 1220, 1256, 1232, 1221, 2061, 1229, 1271, 2058,
     1272, 1274, 2057, 1222, 1231, 1215, 1280, 1284, 2056, 2055,
     1285, 1295, 1288, 2007, 2009, 2006, 2019, 1179, 2004, 2007,
     2004,    0, 2015, 2014, 1996, 1996, 2005, 1991, 1467, 1987,
     1983,    0, 1311, 1538, 1970, 1989,    0, 1316, 1609, 1982,
     1974, 1971, 1963, 1969, 1963, 1982, 1968, 1978, 1973,    0,
     1351, 1680, 1294, 1300, 2004, 1289, 2003, 2002, 1312, 1298,
     1328, 1338, 1340, 1347, 1342, 2001, 2000, 1357, 1343, 1999,
     1998, 1997, 1356, 1996, 1995, 1994, 1355, 1204, 1993, 1368,

     1992, 1353, 1366, 1408, 1990, 1358, 1410, 1987, 1345, 1986,
     1985, 1984, 1983, 1982, 1981, 1411, 1412, 1425, 1934, 1943,
     1946,    0, 1935, 1948,    0, 1932, 1924, 1928, 1936, 1938,
     1928, 1915, 1919,    0, 1377, 1751, 1902, 1912, 1910, 1893,
     1822, 1908, 1908, 1906, 1896, 1446, 1189, 1897, 1896, 1904,
     1409, 1933, 1932, 1414, 1418, 1416, 1415, 1371, 1426, 1931,
     1930, 1929, 1928, 1469, 1424, 1435, 1438, 1927, 1926, 1925,
     1924, 1923, 1445, 1922, 1921, 1379, 1887, 1868, 1871, 1867,
     1874, 1880,    0, 1335,    0, 1865, 1869, 1868, 1441, 1877,
     1875, 1876, 1873,    0, 1495, 1893, 1860, 1872, 1871, 1864,

     1510, 1864, 1857, 1848, 1964, 1842, 1827, 1872, 1871, 1446,
     1484, 1870, 1489, 1490, 1500, 1503, 1869, 1868, 1867, 1876,
        0, 1833, 1823, 1822, 1815, 1816,    0,    0, 1828, 1828,
     1826, 1825, 1824, 1811, 1822, 1820, 1806, 1821, 1822, 1800,
     1452, 1815, 1516, 1798, 1797,    0, 1520, 2035, 2106, 1810,
     1509, 1511, 1507, 1841, 1840, 1517, 1849,    0, 1794,    0,
     1791, 1804,    0, 2177, 1809, 2248, 1804, 1787, 1791, 1800,
     1789, 1527, 1778, 1768, 1777, 1775, 1770, 1755, 1566, 1768,
     1758,    0, 1572, 2319, 1767, 1797, 1796, 1553, 1795,    0,
     1749,    0,    0,    0, 1581, 2390, 1745,    0, 1585, 2461,

     1764, 1745, 1746, 1525, 1748, 1590, 1757, 1745, 1754, 1755,
     1738, 1737, 1750, 1740, 1752, 1753, 1777,    0, 1742, 1722,
     1721, 1739, 1734, 1741, 1727, 1735, 1734, 1594, 1721, 2532,
     1718, 1731, 1721, 1725, 1727, 1714, 1722, 1713, 1712, 1689,
     1697, 1685, 1683, 1692, 1683, 2603, 1598, 1685,    0, 1602,
     2674, 1683, 1683, 1695, 1696, 1681, 1688, 1674, 1668, 1664,
     1674, 1677, 1655, 1662, 1647, 1642,    0, 1626, 2745, 1640,
     1623, 1633, 1619, 1624, 1619, 1602, 1607, 1597, 1599, 2816,
     1587, 1586, 2887, 2958, 3029, 1580, 1581, 1573, 1569, 1552,
     1544, 3100, 1533, 1530, 1524,    0, 1635, 3171, 1515, 1483,

        0, 1639, 3242,    0, 1643, 3313,    0, 1652, 3384, 1473,
     1484, 1482, 1465, 1463, 3455,    0, 1656, 3526, 1405, 1395,
     1401, 1381, 1393, 3597, 1373, 1314, 3668, 1306,    0, 1660,
     3739, 3810, 1319, 3881, 1314, 1230,    0, 1664, 3952, 1174,
     4023,    0, 1668, 4094, 1174,    0, 1672, 4165,  925,    0,
     1697, 4236,   31,   49, 4307,    0, 1706, 4378, 4449,  142,
      467,  700,    0, 1710, 4520,    0, 1714, 4591, 4662,  696,
      714,    0, 1723, 4733,  772, 4804, 4875,    0, 1727, 4946,
        0, 1731, 5017, 5954, 5088, 5100, 5112, 5124, 5136, 5144,
     5155, 5163, 5175, 5178, 5188, 5198, 5206, 5209, 5217, 5227,

     5237, 5245, 5257, 5269, 5281, 5293, 5305, 5317, 5329, 5341,
     5353, 5365, 5377, 5389, 5401, 5413, 5425, 5437, 5449, 5461,
     5473, 5485, 5497, 5509, 5521, 5533, 5545, 5557, 5569, 5581,
     5593, 5605, 5617, 5629, 5641, 5653, 5665, 5677, 5689, 5701,
     5713, 5725, 5737, 5749, 5761, 5773, 5785, 5797, 5809, 5821,
     5833, 5845, 5857, 5869, 5881, 5893, 5905, 5917, 5929, 5941
    } ;

static yyconst flex_int16_t yy_def[1261] =
    {   0,
     1184,    1, 1185, 1185, 1186, 1186, 1187, 1187, 1188, 1188,
        1,   11, 1189, 1189, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1190, 1184, 1184, 1184, 1184, 1184, 1184, 1191, 1184,
     1184, 1184, 1184, 1184, 1192, 1193, 1184, 1194, 1192, 1192,
     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,
     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,
     1184, 1184, 1194, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,   31, 1184,
     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,
     1192, 1184, 1184, 1190, 1190, 1190, 1190, 1190, 1190, 1190,

     1190, 1190, 1190, 1190, 1184, 1184, 1184, 1195, 1196, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1191, 1197, 1184, 1184,
     1184, 1184, 1184,   31, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1192, 1198, 1193, 1184, 1199, 1199, 1199, 1199, 1199,
     1199, 1199, 1199, 1199, 1199, 1199, 1192, 1192, 1192, 1192,
     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,
     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,
     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,
     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,
     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,

     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1184,
     1184, 1184, 1184, 1199, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1190, 1190, 1190, 1190, 1190, 1190, 1190, 1190,
     1190, 1190, 1190, 1190, 1190, 1190, 1190, 1184, 1184, 1184,
     1184, 1184, 1195, 1195, 1196, 1184, 1197, 1184, 1184, 1200,
     1201, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1202, 1199,
     1199, 1199, 1199, 1199, 1199, 1199, 1199, 1199, 1199, 1199,
     1199, 1199, 1199, 1199, 1192, 1192, 1192, 1192, 1192, 1192,
     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,
     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,

     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,
     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,
     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,
     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,
     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,
     1199, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1190, 1190, 1190, 1190, 1190, 1190, 1190, 1190, 1190,
     1190, 1190, 1190, 1190, 1190, 1190, 1190, 1190, 1190, 1184,
     1184, 1184, 1184, 1184, 1200, 1200, 1201, 1184, 1184, 1184,
     1184, 1202, 1199, 1199, 1199, 1199, 1199, 1199, 1199, 1199,

     1199, 1199, 1199, 1199, 1199, 1199, 1199, 1199, 1199, 1199,
     1199, 1199, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,
     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,
     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,
     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,
     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,
     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,
     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,
     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,
     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1199, 1184,

     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1190,
     1190, 1190, 1190, 1190, 1190, 1190, 1190, 1190, 1190, 1190,
     1190, 1190, 1190, 1190, 1190, 1190, 1184, 1184, 1184, 1199,
     1199, 1199, 1199, 1199, 1199, 1203, 1199, 1199, 1199, 1199,
     1199, 1199, 1199, 1199, 1199, 1199, 1199, 1199, 1199, 1199,
     1199, 1199, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,
     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,
     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,
     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,
     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,

     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,
     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,
     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1199, 1190,
     1190, 1190, 1190, 1190, 1190, 1190, 1190, 1190, 1190, 1190,
     1190, 1190, 1190, 1190, 1190, 1184, 1199, 1199, 1199, 1199,
     1199, 1199, 1204, 1204, 1203, 1199, 1205, 1199, 1199, 1206,
     1199, 1199, 1199, 1199, 1199, 1199, 1199, 1199, 1199, 1199,
     1207, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,
     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,
     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,

     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,
     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,
     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,
     1192, 1192, 1184, 1190, 1190, 1190, 1190, 1190, 1190, 1190,
     1190, 1190, 1190, 1190, 1199, 1199, 1199, 1199, 1208, 1199,
     1199, 1209, 1209, 1205, 1199, 1199, 1210, 1210, 1206, 1199,
     1199, 1199, 1199, 1199, 1199, 1199, 1199, 1199, 1199, 1211,
     1211, 1207, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,
     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,
     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,

     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,
     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1184, 1190, 1190,
     1190, 1190, 1190, 1190, 1190, 1190, 1190, 1190, 1190, 1199,
     1199, 1199, 1199, 1212, 1212, 1208, 1199, 1199, 1199, 1199,
     1213, 1199, 1199, 1199, 1199, 1199, 1199, 1199, 1199, 1199,
     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,
     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192,
     1192, 1192, 1192, 1192, 1192, 1184, 1190, 1190, 1190, 1190,
     1190, 1190, 1190, 1190, 1190, 1199, 1199, 1199, 1199, 1199,
     1199, 1199, 1199, 1214, 1214, 1213, 1199, 1199, 1199, 1199,

     1184, 1199, 1199, 1199, 1215, 1199, 1199, 1192, 1192, 1192,
     1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1192, 1216,
     1190, 1190, 1190, 1190, 1190, 1190, 1190, 1190, 1199, 1199,
     1199, 1199, 1199, 1199, 1199, 1199, 1199, 1199, 1199, 1199,
     1199, 1199, 1199, 1199, 1199, 1217, 1217, 1215, 1218, 1199,
     1192, 1192, 1192, 1192, 1192, 1192, 1216, 1219, 1190, 1190,
     1190, 1190, 1190, 1220, 1199, 1221, 1199, 1199, 1199, 1199,
     1199, 1199, 1199, 1199, 1199, 1199, 1199, 1199, 1184, 1199,
     1199, 1222, 1222, 1218, 1199, 1192, 1192, 1192, 1192, 1219,
     1190, 1190, 1190, 1223, 1223, 1220, 1199, 1224, 1224, 1221,

     1199, 1199, 1199, 1199, 1199, 1184, 1199, 1199, 1199, 1199,
     1199, 1199, 1199, 1199, 1199, 1199, 1192, 1190, 1199, 1199,
     1199, 1199, 1199, 1199, 1199, 1199, 1199, 1199, 1199, 1225,
     1199, 1199, 1199, 1199, 1199, 1199, 1199, 1199, 1199, 1199,
     1199, 1199, 1199, 1199, 1199, 1226, 1184, 1199, 1227, 1227,
     1225, 1199, 1199, 1199, 1199, 1199, 1199, 1199, 1199, 1199,
     1199, 1199, 1199, 1199, 1199, 1199, 1228, 1228, 1226, 1199,
     1199, 1199, 1199, 1199, 1199, 1199, 1199, 1199, 1199, 1229,
     1199, 1199, 1230, 1231, 1232, 1199, 1199, 1199, 1199, 1199,
     1199, 1233, 1199, 1199, 1199, 1234, 1234, 1229, 1199, 1199,

     1235, 1235, 1230, 1236, 1236, 1231, 1237, 1237, 1232, 1199,
     1199, 1199, 1199, 1199, 1238, 1239, 1239, 1233, 1199, 1199,
     1199, 1199, 1199, 1240, 1199, 1199, 1241, 1199, 1242, 1242,
     1238, 1243, 1199, 1244, 1199, 1199, 1245, 1245, 1240, 1199,
     1246, 1247, 1247, 1241, 1199, 1248, 1248, 1243, 1199, 1249,
     1249, 1244, 1199, 1199, 1250, 1251, 1251, 1246, 1252, 1199,
     1199, 1199, 1253, 1253, 1250, 1254, 1254, 1252, 1255, 1199,
     1199, 1256, 1256, 1255, 1199, 1257, 1258, 1259, 1259, 1257,
     1260, 1260, 1258,    0, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,

     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184
    } ;

static yyconst flex_uint16_t yy_nxt[6026] =
    {   0,
       16,   17,   17,   18,   17,   19,   20,   21,   22,   23,
       24,   25,   21,   26,   27,   28,   29,   30,   31,   31,
       31,   31,   21,   32,   33,   34,   21,   35,   35,   35,
       35,   35,   35,   35,   35,   35,   35,   35,   35,   35,
       36,   37,   35,   38,   39,   40,   41,   42,   43,   44,
       45,   46,   47,   48,   35,   49,   50,   51,   52,   53,
       54,   55,   56,   57,   58,   59,   60,   35,   35,   61,
       62,   65,   65,   68,   68,   73,   73,  112,   74,   74,
      112,   66,   66,   69,   69,   95,  119,   96, 1161,  113,
      120,   97,  113,   98,  115,   99,  100,  116,  101,  126,

      127,  102,  103,  133,  104,  117,  117,  133,  117,  128,
      129, 1162,   75,   75,   76,  133,   77,  133,   78,  130,
      131,   79,   79,  106,  107,  133,  133,  133,  108,   80,
      109,   81,  110,  115,   82,  151,  116,   83,  133,   84,
      106,   85,  107,  153,  133,  228,  108,  154,   86,  162,
      152,  229,   87,  109,  163,  155,  110,  156,  158,  168,
       88,  157,   89,   90,  133,  147,  159,  148,  160,   91,
      121,  149,  121,  150,  165,  161,  164,  137,  122,  138,
      139,  140,  133,  133,  123,  141,  124,  124,  124,  124,
      142,  133,  143,  144,  133,  145,  146,  166,  125,  133,

      133,  211,  133, 1169,  243,  167,  243,  245,  133,  245,
      124,  169,  117,  117,  202,  117,  125,  365,  133,  170,
      133,  641,  172,  133,  642,  171,  173,  175,  181,  133,
      366,  176,  186,  212,  187,  179,  174,  133,  180,  177,
      182,  178,  183,  188,  184,  189,  197,  185,  190,  200,
      191,  203,  133,  201,  198,  204,  192,  133,  205,  206,
      193,  213,  199,  194,  133,  207,  195,  196,  133,  151,
      208,  209,  137,  111,  138,  139,  140,  133,  218,  219,
      141,  133,  133,  214,  152,  142,  220,  143,  144,  268,
      145,  146,  175,  121,  162,  121,  176,  269,  186,  163,

      187,  122,  277,  261,  177,  181,  178,  133,  133,  188,
      133,  189,  133,  239,  190,  239,  133,  182,  133,  183,
      262,  184,  208,  209,  185,  232,  221,  233,  133,  240,
      240,  275,  234,  235,  133,  133,  133,  240,  241,  276,
      241,  236,  253,  253,  253,  253,  282,  290,  483,  240,
      240,  133,  279,  240,  242,  242,  242,  242,  278,  106,
      107,  280,  133,  133,  108,  133,  109,  254,  254,  133,
      133,  255,  255,  255,  255,  293,  106,  240,  107,  240,
      281,  287,  108,  248,  249,  265,  133,  266,  250,  109,
      251,  133,  252,  283,  133,  133,  267,  288,  284,  133,

      248,  291,  249,  285,  133,  133,  250,  289,  133,  292,
      133,  133,  133,  251,  133,  286,  252,  133,  133,  133,
      133,  133,  133,  133,  133,  295,  133,  294,  133,  133,
      133,  303,  133,  133,  297,  296,  308,  305,  133,  133,
      298,  306,  307,  304,  133,  309,  299,  300,  133,  301,
      302,  310,  311,  133,  329,  133,  133,  319,  133,  312,
      133,  133,  330,  313,  317,  314,  315,  316,  320,  318,
      321,  133,  322,  326,  133,  133,  328,  133,  323,  327,
      332,  331,  333,  324,  133,  335,  338,  325,  133,  133,
      352,  133,  133,  354,  339,  360,  361,  408,  334,  374,

      337,  336,  409,  341,  342,  345,  340,  356,  357,  370,
      371,  343,  239,  375,  239,  358,  344, 1170,  240,  240,
      346,  396,  380,  380,  380,  380,  240,  397,  240,  240,
      411,  412,  347,  348,  349,  350,  240,  353,  240,  240,
      133,  355,  240,  241,  133,  241,  380,  133,  240,  240,
      133,  243,  240,  243, 1184,  359, 1184,  133,  133,  242,
      242,  242,  242,  245,  133,  245,  240,  133,  240,  246,
      246,  246,  385,  387,  385,  387,  240,  246,  240,  383,
      414,  383,  415,  416,  381,  413,  381,  133,  133,  246,
      246,  435,  133,  246,  417,  384,  384,  384,  384,  418,

      382,  382,  389,  389,  389,  389,  248,  249,  382,  420,
      133,  250,  403,  251,  390,  422,  404,  246,  133,  246,
      382,  382,  439,  248,  382,  249,  389,  419,  405,  250,
      423,  406,  390,  133,  133,  133,  251,  255,  255,  255,
      255,  391,  391,  391,  391,  133,  133,  133,  382,  133,
      382,  133,  133,  133,  424,  421,  133,  133,  133,  133,
      133,  425,  133,  133,  133,  391,  426,  427,  133,  133,
      133,  133,  133,  133,  133,  428,  133,  133,  429,  430,
      133,  436,  442,  437,  440,  431,  133,  455,  432,  438,
      433,  434,  446,  444,  452,  443,  447,  133,  451,  457,

      441,  445,  450,  448,  449,  459,  133,  453,  456,  460,
      133,  133,  454,  133,  463,  468,  458,  461,  465,  133,
      467,  133,  462,  133,  133,  464,  133,  133,  133,  133,
      133,  133,  133,  133,  133,  133,  486,  487,  466,  133,
      133,  133,  133,  133,  133,  471,  133,  133, 1171,  133,
      470,  472,  469,  385,  475,  385,  518, 1184, 1175, 1184,
      479, 1176,  488,  473,  133,  474,  476,  481,  478,  477,
      519,  482,  484,  480,  485,  491,  489,  494,  490,  497,
      495,  492,  496,  380,  380,  380,  380,  493,  498,  381,
      387,  381,  387,  533,  382,  382,  527,  527,  527,  527,

      553,  534,  382,  537,  555,  382,  382,  380,  133,  538,
      133,  133,  133,  382,  382,  382,  539,  383,  382,  383,
      527,  133,  133, 1177,  133,  382,  382,  133,  133,  382,
      388,  388,  388,  384,  384,  384,  384,  133,  388,  562,
      133,  564,  382,  557,  382,  389,  389,  389,  389,  556,
      388,  388,  133,  382,  388,  382,  554,  390,  528,  528,
      568,  563,  529,  529,  529,  529,  560,  133,  565,  389,
      391,  391,  391,  391,  558,  390,  559,  133,  388,  133,
      388,  133,  566,  133,  561,  133,  133,  133,  133,  133,
      133,  133,  133,  133,  391,  133,  133,  133,  133,  567,

      133,  133,  133,  133,  133,  133,  133,  133,  133,  569,
      133,  572,  133,  133,  133,  133,  133,  574,  578,  133,
      133,  570,  571,  133,  133,  133,  586,  573,  581,  583,
      590,  133,  577,  576,  575,  587,  580,  588,  582,  589,
      596,  579,  584,  585,  595,  591,  592,  593,  607,  594,
      599,  600,  598,  608,  133,  597,  601,  602,  133,  605,
      133,  133,  606,  133,  133,  133,  133,  133,  609,  610,
      133,  133,  611,  133,  133,  133,  133,  133,  133,  626,
      627,  529,  529,  529,  529,  603,  612, 1160,  527,  527,
      527,  527,  613,  646,  646,  646,  646,  133,  619,  615,

      614,  604,  133,  620,  618,  617,  616,  623,  624,  133,
      628,  621,  527,  133,  673,  622,  133,  646,  625,  653,
      654,  654,  133,  654,  653,  653,  653,  133,  653,  653,
      653,  653,  653,  653,  653,  653,  653,  674,  680,  133,
      133,  653,  653,  653,  653,  653,  133,  672,  675,  676,
      133,  133,  677,  133,  133,  133,  133,  679,  133,  653,
      653,  678,  653,  133,  133,  133,  681,  133,  133,  133,
      695,  696,  133,  133,  133,  133,  133,  133,  133,  133,
      133,  683,  689,  133,  682,  686,  133,  688,  653,  653,
      133,  684,  685,  693,  687,  691,  133,  701,  690,  133,

      692,  697,  700,  133,  699,  710,  711,  702,  133,  133,
      133,  694,  708,  133,  133,  133,  698,  133,  133,  706,
      705,  703,  704,  707,  709,  133,  133,  133,  713,  133,
      133,  133,  712,  714,  133,  133,  133,  133,  133,  133,
      133,  133,  646,  646,  646,  646,  725,  133,  723,  718,
      133,  716,  133,  717,  715,  654,  654,  133,  654,  719,
      724,  720,  721,  726,  728,  729,  646,  776,  133,  722,
      730,  774,  133,  777,  133,  133,  731,  133,  133,  778,
      780,  727,  732,  653,  653,  653,  773,  653,  653,  653,
      653,  785,  653,  653,  653,  653,  653,  653,  653,  653,

      653,  779,  133,  775,  781,  653,  653,  653,  653,  653,
      133,  133,  133,  133,  133,  783,  786,  133,  133,  788,
      133,  782, 1159,  653,  653,  823,  653,  133,  133,  133,
      133,  133,  784,  810,  811, 1155,  133,  133,  133,  903,
      797,  133,  791,  824,  793,  133,  904,  133,  133,  799,
      792,  865,  653,  653,  752,  753,  753,  787,  753,  752,
      752,  752,  794,  752,  752,  752,  752,  752,  752,  752,
      752,  752,  133,  789,  790,  795,  752,  752,  752,  752,
      752,  796,  800,  798,  802,  804,  803,  133,  133,  808,
      133,  809,  733, 1154,  752,  752,  133,  752,  812,  813,

      133,  133,  814,  815,  801,  133,  818,  818,  818,  818,
      133,  133,  753,  753,  133,  753,  133,  758,  758,  805,
      758,  806,  807,  752,  752,  757,  758,  758,  133,  758,
      757,  757,  757,  816,  757,  757,  757,  757,  757,  757,
      757,  757,  757,  817,  133,  853,  851,  757,  757,  757,
      757,  757,  771,  771,  133,  771,  133,  852,  133,  133,
      855,  133, 1153,  133,  854,  757,  757, 1149,  757,  133,
     1145,  133,  133,  133,  133, 1141,  870,  871,  835,  835,
      927,  835,  133,  876,  133,  920,  928,  133,  856,  873,
      860,  862,  858,  857,  757,  757,  770,  771,  771,  859,

      771,  770,  770,  770,  863,  770,  770,  770,  770,  770,
      770,  770,  770,  770,  861,  864,  866,  867,  770,  770,
      770,  770,  770,  868,  133,  133,  133,  133,  133,  876,
      133,  133,  133,  913,  133, 1140,  770,  770, 1136,  770,
      133, 1135,  133,  818,  818,  818,  818,  901,  901, 1134,
      901,  133, 1133, 1132,  133,  908,  869,  872,  874,  875,
      911,  133,  133,  912,  909,  770,  770,  834,  835,  835,
      910,  835,  834,  834,  834,  914,  834,  834,  834,  834,
      834,  834,  834,  834,  834,  133,  918,  916,  932,  834,
      834,  834,  834,  834,  902,  917,  895,  895,  933,  895,

      133,  919,  976,  934,  951,  133,  133,  834,  834,  977,
      834,  901,  901,  915,  901, 1128,  133,  979,  979,  133,
      979,  947,  947,  133,  947,  133, 1127,  133, 1006, 1006,
     1126, 1006, 1125,  133, 1124, 1123,  834,  834,  752,  752,
      752,  953,  752,  752,  752,  752,  952,  752,  752,  752,
      752,  752,  752,  752,  752,  752,  955,  954,  956,  987,
      752,  752,  752,  752,  752,  989,  986,  979,  979,  133,
      979,  988, 1023,  983,  983, 1007,  983, 1122,  752,  752,
     1121,  752,  995,  995, 1024,  995,  999,  999, 1025,  999,
     1120, 1006, 1006, 1026, 1006, 1047, 1047, 1119, 1047, 1047,

     1047, 1017, 1047, 1050, 1050, 1115, 1050,  752,  752,  757,
      757,  757, 1114,  757,  757,  757,  757, 1113,  757,  757,
      757,  757,  757,  757,  757,  757,  757, 1068, 1068, 1112,
     1068,  757,  757,  757,  757,  757, 1097, 1097, 1111, 1097,
     1102, 1102, 1110, 1102, 1105, 1105, 1100, 1105, 1099,  757,
      757, 1095,  757, 1108, 1108, 1094, 1108, 1117, 1117, 1093,
     1117, 1130, 1130, 1092, 1130, 1138, 1138, 1091, 1138, 1143,
     1143, 1090, 1143, 1147, 1147, 1089, 1147, 1088,  757,  757,
      770,  770,  770, 1087,  770,  770,  770,  770, 1086,  770,
      770,  770,  770,  770,  770,  770,  770,  770, 1151, 1151,

     1085, 1151,  770,  770,  770,  770,  770, 1157, 1157, 1084,
     1157, 1164, 1164, 1083, 1164, 1167, 1167, 1082, 1167, 1081,
      770,  770, 1080,  770, 1173, 1173, 1079, 1173, 1179, 1179,
     1078, 1179, 1182, 1182, 1077, 1182, 1076, 1075, 1074, 1073,
     1072, 1071, 1070, 1066, 1065, 1064, 1063, 1062, 1061,  770,
      770,  834,  834,  834, 1060,  834,  834,  834,  834, 1059,
      834,  834,  834,  834,  834,  834,  834,  834,  834, 1058,
     1057, 1056, 1055,  834,  834,  834,  834,  834, 1054, 1053,
     1052, 1048, 1046, 1045, 1044, 1043, 1042, 1041, 1040, 1039,
     1038,  834,  834,  133,  834, 1037, 1036, 1035, 1034, 1033,

     1032, 1031, 1030, 1029, 1028, 1027, 1022, 1021, 1020, 1019,
     1018,  133,  133,  133, 1016, 1015, 1014, 1013, 1012, 1011,
      834,  834,  894,  895,  895, 1010,  895,  894,  894,  894,
     1009,  894,  894,  894,  894,  894,  894,  894,  894,  894,
     1008, 1005, 1004, 1003,  894,  894,  894,  894,  894, 1002,
     1001,  997,  993,  992,  991,  958,  133,  133,  985,  981,
      980,  978,  894,  894,  975,  894,  974,  973,  972,  971,
      970,  969,  968,  967,  966,  965,  964,  963,  962,  961,
      960,  959,  958,  133,  133,  133,  133,  133,  133,  950,
      949,  894,  894,  894,  894,  894,  945,  894,  894,  894,

      894,  944,  894,  894,  894,  894,  894,  894,  894,  894,
      894,  943,  942,  941,  940,  894,  894,  894,  894,  894,
      939,  938,  937,  936,  935,  931,  930,  929,  926,  925,
      924,  923,  922,  894,  894,  921,  894,  133,  133,  133,
      133,  133,  133,  133,  133,  133,  133,  133,  133,  133,
      907,  906,  905,  900,  899,  898,  897,  893,  892,  891,
      890,  889,  894,  894,  946,  947,  947,  888,  947,  946,
      946,  946,  887,  946,  946,  946,  946,  946,  946,  946,
      946,  946,  886,  885,  884,  883,  946,  946,  946,  946,
      946,  882,  881,  880,  879,  878,  877,  133,  133,  133,

      133,  133,  133,  133,  946,  946,  133,  946,  133,  133,
      133,  133,  133,  133,  133,  133,  133,  133,  133,  133,
      133,  850,  849,  848,  847,  846,  845,  844,  843,  842,
      841,  840,  839,  946,  946,  946,  946,  946,  838,  946,
      946,  946,  946,  837,  946,  946,  946,  946,  946,  946,
      946,  946,  946,  833,  832,  831,  830,  946,  946,  946,
      946,  946,  829,  828,  827,  826,  825,  822,  821,  820,
      819,  133,  133,  133,  133,  946,  946,  133,  946,  133,
      133,  133,  133,  133,  133,  133,  133,  133,  133,  133,
      133,  133,  133,  769,  768,  767,  766,  765,  764,  763,

      762,  761,  760,  756,  946,  946,  982,  983,  983,  755,
      983,  982,  982,  982,  751,  982,  982,  982,  982,  982,
      982,  982,  982,  982,  750,  749,  748,  747,  982,  982,
      982,  982,  982,  746,  745,  744,  743,  742,  741,  740,
      739,  738,  737,  736,  735,  734,  982,  982,  733,  982,
      133,  133,  133,  133,  133,  133,  133,  133,  133,  133,
      133,  133,  133,  133,  133,  133,  133,  133,  133,  671,
      670,  669,  668,  667,  666,  982,  982,  994,  995,  995,
      665,  995,  994,  994,  994,  664,  994,  994,  994,  994,
      994,  994,  994,  994,  994,  663,  662,  661,  660,  994,

      994,  994,  994,  994,  659,  658,  657,  656,  652,  651,
      650,  649,  648,  647,  645,  644,  643,  994,  994,  640,
      994,  639,  638,  637,  636,  635,  634,  633,  632,  631,
      630,  629,  133,  133,  133,  133,  133,  133,  133,  133,
      133,  133,  133,  133,  133,  133,  994,  994,  998,  999,
      999,  133,  999,  998,  998,  998,  133,  998,  998,  998,
      998,  998,  998,  998,  998,  998,  133,  552,  551,  550,
      998,  998,  998,  998,  998,  549,  548,  547,  546,  545,
      544,  543,  542,  541,  540,  536,  535,  532,  998,  998,
      531,  998,  530,  133,  526,  525,  524,  523,  522,  521,

      520,  517,  516,  515,  514,  513,  512,  511,  510,  509,
      508,  507,  506,  505,  504,  503,  502,  998,  998,  982,
      982,  982,  501,  982,  982,  982,  982,  500,  982,  982,
      982,  982,  982,  982,  982,  982,  982,  499,  133,  133,
      133,  982,  982,  982,  982,  982,  133,  133,  133,  410,
      407,  402,  401,  400,  399,  398,  395,  394,  393,  982,
      982,  133,  982,  379,  378,  377,  376,  373,  372,  369,
      368,  367,  364,  363,  362,  351,  133,  274,  273,  272,
      271,  270,  264,  263,  133,  258,  257,  256,  982,  982,
      994,  994,  994,  238,  994,  994,  994,  994,  237,  994,

      994,  994,  994,  994,  994,  994,  994,  994,  231,  230,
      227,  226,  994,  994,  994,  994,  994,  225,  224,  223,
      222,  133,  133,  133,  133,  133,  217,  216,  215,  210,
      994,  994,  135,  994,  133,  114,  111,  105,   93, 1184,
       63,   71,   71,   63, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,  994,
      994,  998,  998,  998, 1184,  998,  998,  998,  998, 1184,
      998,  998,  998,  998,  998,  998,  998,  998,  998, 1184,
     1184, 1184, 1184,  998,  998,  998,  998,  998, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,

     1184,  998,  998, 1184,  998, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
      998,  998, 1049, 1050, 1050, 1184, 1050, 1049, 1049, 1049,
     1184, 1049, 1049, 1049, 1049, 1049, 1049, 1049, 1049, 1049,
     1184, 1184, 1184, 1184, 1049, 1049, 1049, 1049, 1049, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1049, 1049, 1184, 1049, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,

     1184, 1049, 1049, 1067, 1068, 1068, 1184, 1068, 1067, 1067,
     1067, 1184, 1067, 1067, 1067, 1067, 1067, 1067, 1067, 1067,
     1067, 1184, 1184, 1184, 1184, 1067, 1067, 1067, 1067, 1067,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1067, 1067, 1184, 1067, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1067, 1067, 1049, 1049, 1049, 1184, 1049, 1049,
     1049, 1049, 1184, 1049, 1049, 1049, 1049, 1049, 1049, 1049,
     1049, 1049, 1184, 1184, 1184, 1184, 1049, 1049, 1049, 1049,

     1049, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1049, 1049, 1184, 1049, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1049, 1049, 1067, 1067, 1067, 1184, 1067,
     1067, 1067, 1067, 1184, 1067, 1067, 1067, 1067, 1067, 1067,
     1067, 1067, 1067, 1184, 1184, 1184, 1184, 1067, 1067, 1067,
     1067, 1067, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1067, 1067, 1184, 1067, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,

     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1067, 1067, 1096, 1097, 1097, 1184,
     1097, 1096, 1096, 1096, 1184, 1096, 1096, 1096, 1096, 1096,
     1096, 1096, 1096, 1096, 1184, 1184, 1184, 1184, 1096, 1096,
     1096, 1096, 1096, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1096, 1096, 1184, 1096,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1096, 1096, 1101, 1102, 1102,
     1184, 1102, 1101, 1101, 1101, 1184, 1101, 1101, 1101, 1101,

     1101, 1101, 1101, 1101, 1101, 1184, 1184, 1184, 1184, 1101,
     1101, 1101, 1101, 1101, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1101, 1101, 1184,
     1101, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1101, 1101, 1104, 1105,
     1105, 1184, 1105, 1104, 1104, 1104, 1184, 1104, 1104, 1104,
     1104, 1104, 1104, 1104, 1104, 1104, 1184, 1184, 1184, 1184,
     1104, 1104, 1104, 1104, 1104, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1104, 1104,

     1184, 1104, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1104, 1104, 1107,
     1108, 1108, 1184, 1108, 1107, 1107, 1107, 1184, 1107, 1107,
     1107, 1107, 1107, 1107, 1107, 1107, 1107, 1184, 1184, 1184,
     1184, 1107, 1107, 1107, 1107, 1107, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1107,
     1107, 1184, 1107, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1107, 1107,

     1116, 1117, 1117, 1184, 1117, 1116, 1116, 1116, 1184, 1116,
     1116, 1116, 1116, 1116, 1116, 1116, 1116, 1116, 1184, 1184,
     1184, 1184, 1116, 1116, 1116, 1116, 1116, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1116, 1116, 1184, 1116, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1116,
     1116, 1096, 1096, 1096, 1184, 1096, 1096, 1096, 1096, 1184,
     1096, 1096, 1096, 1096, 1096, 1096, 1096, 1096, 1096, 1184,
     1184, 1184, 1184, 1096, 1096, 1096, 1096, 1096, 1184, 1184,

     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1096, 1096, 1184, 1096, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1096, 1096, 1101, 1101, 1101, 1184, 1101, 1101, 1101, 1101,
     1184, 1101, 1101, 1101, 1101, 1101, 1101, 1101, 1101, 1101,
     1184, 1184, 1184, 1184, 1101, 1101, 1101, 1101, 1101, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1101, 1101, 1184, 1101, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,

     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1101, 1101, 1104, 1104, 1104, 1184, 1104, 1104, 1104,
     1104, 1184, 1104, 1104, 1104, 1104, 1104, 1104, 1104, 1104,
     1104, 1184, 1184, 1184, 1184, 1104, 1104, 1104, 1104, 1104,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1104, 1104, 1184, 1104, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1104, 1104, 1107, 1107, 1107, 1184, 1107, 1107,
     1107, 1107, 1184, 1107, 1107, 1107, 1107, 1107, 1107, 1107,

     1107, 1107, 1184, 1184, 1184, 1184, 1107, 1107, 1107, 1107,
     1107, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1107, 1107, 1184, 1107, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1107, 1107, 1129, 1130, 1130, 1184, 1130,
     1129, 1129, 1129, 1184, 1129, 1129, 1129, 1129, 1129, 1129,
     1129, 1129, 1129, 1184, 1184, 1184, 1184, 1129, 1129, 1129,
     1129, 1129, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1129, 1129, 1184, 1129, 1184,

     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1129, 1129, 1116, 1116, 1116, 1184,
     1116, 1116, 1116, 1116, 1184, 1116, 1116, 1116, 1116, 1116,
     1116, 1116, 1116, 1116, 1184, 1184, 1184, 1184, 1116, 1116,
     1116, 1116, 1116, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1116, 1116, 1184, 1116,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1116, 1116, 1137, 1138, 1138,

     1184, 1138, 1137, 1137, 1137, 1184, 1137, 1137, 1137, 1137,
     1137, 1137, 1137, 1137, 1137, 1184, 1184, 1184, 1184, 1137,
     1137, 1137, 1137, 1137, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1137, 1137, 1184,
     1137, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1137, 1137, 1142, 1143,
     1143, 1184, 1143, 1142, 1142, 1142, 1184, 1142, 1142, 1142,
     1142, 1142, 1142, 1142, 1142, 1142, 1184, 1184, 1184, 1184,
     1142, 1142, 1142, 1142, 1142, 1184, 1184, 1184, 1184, 1184,

     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1142, 1142,
     1184, 1142, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1142, 1142, 1129,
     1129, 1129, 1184, 1129, 1129, 1129, 1129, 1184, 1129, 1129,
     1129, 1129, 1129, 1129, 1129, 1129, 1129, 1184, 1184, 1184,
     1184, 1129, 1129, 1129, 1129, 1129, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1129,
     1129, 1184, 1129, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,

     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1129, 1129,
     1146, 1147, 1147, 1184, 1147, 1146, 1146, 1146, 1184, 1146,
     1146, 1146, 1146, 1146, 1146, 1146, 1146, 1146, 1184, 1184,
     1184, 1184, 1146, 1146, 1146, 1146, 1146, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1146, 1146, 1184, 1146, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1146,
     1146, 1150, 1151, 1151, 1184, 1151, 1150, 1150, 1150, 1184,
     1150, 1150, 1150, 1150, 1150, 1150, 1150, 1150, 1150, 1184,

     1184, 1184, 1184, 1150, 1150, 1150, 1150, 1150, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1150, 1150, 1184, 1150, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1150, 1150, 1137, 1137, 1137, 1184, 1137, 1137, 1137, 1137,
     1184, 1137, 1137, 1137, 1137, 1137, 1137, 1137, 1137, 1137,
     1184, 1184, 1184, 1184, 1137, 1137, 1137, 1137, 1137, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1137, 1137, 1184, 1137, 1184, 1184, 1184, 1184,

     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1137, 1137, 1156, 1157, 1157, 1184, 1157, 1156, 1156,
     1156, 1184, 1156, 1156, 1156, 1156, 1156, 1156, 1156, 1156,
     1156, 1184, 1184, 1184, 1184, 1156, 1156, 1156, 1156, 1156,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1156, 1156, 1184, 1156, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1156, 1156, 1142, 1142, 1142, 1184, 1142, 1142,

     1142, 1142, 1184, 1142, 1142, 1142, 1142, 1142, 1142, 1142,
     1142, 1142, 1184, 1184, 1184, 1184, 1142, 1142, 1142, 1142,
     1142, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1142, 1142, 1184, 1142, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1142, 1142, 1146, 1146, 1146, 1184, 1146,
     1146, 1146, 1146, 1184, 1146, 1146, 1146, 1146, 1146, 1146,
     1146, 1146, 1146, 1184, 1184, 1184, 1184, 1146, 1146, 1146,
     1146, 1146, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,

     1184, 1184, 1184, 1184, 1184, 1146, 1146, 1184, 1146, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1146, 1146, 1150, 1150, 1150, 1184,
     1150, 1150, 1150, 1150, 1184, 1150, 1150, 1150, 1150, 1150,
     1150, 1150, 1150, 1150, 1184, 1184, 1184, 1184, 1150, 1150,
     1150, 1150, 1150, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1150, 1150, 1184, 1150,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,

     1184, 1184, 1184, 1184, 1184, 1150, 1150, 1163, 1164, 1164,
     1184, 1164, 1163, 1163, 1163, 1184, 1163, 1163, 1163, 1163,
     1163, 1163, 1163, 1163, 1163, 1184, 1184, 1184, 1184, 1163,
     1163, 1163, 1163, 1163, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1163, 1163, 1184,
     1163, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1163, 1163, 1156, 1156,
     1156, 1184, 1156, 1156, 1156, 1156, 1184, 1156, 1156, 1156,
     1156, 1156, 1156, 1156, 1156, 1156, 1184, 1184, 1184, 1184,

     1156, 1156, 1156, 1156, 1156, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1156, 1156,
     1184, 1156, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1156, 1156, 1166,
     1167, 1167, 1184, 1167, 1166, 1166, 1166, 1184, 1166, 1166,
     1166, 1166, 1166, 1166, 1166, 1166, 1166, 1184, 1184, 1184,
     1184, 1166, 1166, 1166, 1166, 1166, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1166,
     1166, 1184, 1166, 1184, 1184, 1184, 1184, 1184, 1184, 1184,

     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1166, 1166,
     1163, 1163, 1163, 1184, 1163, 1163, 1163, 1163, 1184, 1163,
     1163, 1163, 1163, 1163, 1163, 1163, 1163, 1163, 1184, 1184,
     1184, 1184, 1163, 1163, 1163, 1163, 1163, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1163, 1163, 1184, 1163, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1163,
     1163, 1166, 1166, 1166, 1184, 1166, 1166, 1166, 1166, 1184,

     1166, 1166, 1166, 1166, 1166, 1166, 1166, 1166, 1166, 1184,
     1184, 1184, 1184, 1166, 1166, 1166, 1166, 1166, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1166, 1166, 1184, 1166, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1166, 1166, 1172, 1173, 1173, 1184, 1173, 1172, 1172, 1172,
     1184, 1172, 1172, 1172, 1172, 1172, 1172, 1172, 1172, 1172,
     1184, 1184, 1184, 1184, 1172, 1172, 1172, 1172, 1172, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,

     1184, 1184, 1172, 1172, 1184, 1172, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1172, 1172, 1172, 1172, 1172, 1184, 1172, 1172, 1172,
     1172, 1184, 1172, 1172, 1172, 1172, 1172, 1172, 1172, 1172,
     1172, 1184, 1184, 1184, 1184, 1172, 1172, 1172, 1172, 1172,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1172, 1172, 1184, 1172, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,

     1184, 1184, 1172, 1172, 1178, 1179, 1179, 1184, 1179, 1178,
     1178, 1178, 1184, 1178, 1178, 1178, 1178, 1178, 1178, 1178,
     1178, 1178, 1184, 1184, 1184, 1184, 1178, 1178, 1178, 1178,
     1178, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1178, 1178, 1184, 1178, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1178, 1178, 1181, 1182, 1182, 1184, 1182,
     1181, 1181, 1181, 1184, 1181, 1181, 1181, 1181, 1181, 1181,
     1181, 1181, 1181, 1184, 1184, 1184, 1184, 1181, 1181, 1181,

     1181, 1181, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1181, 1181, 1184, 1181, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1181, 1181, 1178, 1178, 1178, 1184,
     1178, 1178, 1178, 1178, 1184, 1178, 1178, 1178, 1178, 1178,
     1178, 1178, 1178, 1178, 1184, 1184, 1184, 1184, 1178, 1178,
     1178, 1178, 1178, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1178, 1178, 1184, 1178,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,

     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1178, 1178, 1181, 1181, 1181,
     1184, 1181, 1181, 1181, 1181, 1184, 1181, 1181, 1181, 1181,
     1181, 1181, 1181, 1181, 1181, 1184, 1184, 1184, 1184, 1181,
     1181, 1181, 1181, 1181, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1181, 1181, 1184,
     1181, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1181, 1181,   64,   64,
       64,   64,   64,   64,   64,   64,   64,   64,   64,   64,

       67,   67,   67,   67,   67,   67,   67,   67,   67,   67,
       67,   67,   70,   70,   70,   70,   70,   70,   70,   70,
       70,   70,   70,   70,   72,   72,   72,   72,   72,   72,
       72,   72,   72,   72,   72,   72,   92,   92,   92,   92,
       92,   92,   92,   92,   92,   92,   92,   92,   94, 1184,
       94,   94, 1184,   94,   94,   94,  118,  118, 1184, 1184,
     1184, 1184, 1184, 1184,  118,  118,  118,  132,  132,  132,
      132, 1184,  132,  132,  132,  134, 1184, 1184,  134,  134,
      134,  134,  134,  134,  134,  134,  134,  136,  136,  136,
      244, 1184, 1184, 1184,  244,  244,  244,  244, 1184,  244,

      246, 1184, 1184, 1184,  246, 1184,  246, 1184, 1184,  246,
      247, 1184,  247,  247, 1184,  247,  247,  247,  259,  259,
      259,  260, 1184,  260,  260, 1184,  260,  260,  260,  386,
     1184, 1184, 1184,  386,  386,  386,  386, 1184,  386,  388,
     1184, 1184, 1184,  388, 1184,  388, 1184, 1184,  388,  392,
      392,  392,  392, 1184,  392,  392,  392,  655,  655,  655,
     1184,  655,  655,  655,  655,  655,  655,  655,  655,  653,
      653,  653, 1184,  653,  653,  653,  653,  653,  653,  653,
      653,  754,  754,  754, 1184,  754,  754,  754,  754,  754,
      754,  754,  754,  759,  759,  759, 1184,  759,  759,  759,

      759,  759,  759,  759,  759,  772,  772,  772, 1184,  772,
      772,  772,  772,  772,  772,  772,  772,  836,  836,  836,
     1184,  836,  836,  836,  836,  836,  836,  836,  836,  752,
      752,  752, 1184,  752,  752,  752,  752,  752,  752,  752,
      752,  757,  757,  757, 1184,  757,  757,  757,  757,  757,
      757,  757,  757,  770,  770,  770, 1184,  770,  770,  770,
      770,  770,  770,  770,  770,  834,  834,  834, 1184,  834,
      834,  834,  834,  834,  834,  834,  834,  896,  896,  896,
     1184,  896,  896,  896,  896,  896,  896,  896,  896,  894,
      894,  894, 1184,  894,  894,  894,  894,  894,  894,  894,

      894,  948,  948,  948, 1184,  948,  948,  948,  948,  948,
      948,  948,  948,  957,  957,  957,  957,  957,  957,  957,
      957,  957,  957,  957,  957,  946,  946,  946, 1184,  946,
      946,  946,  946,  946,  946,  946,  946,  984,  984,  984,
     1184,  984,  984,  984,  984,  984,  984,  984,  984,  990,
      990,  990, 1184,  990,  990,  990,  990,  990,  990,  990,
      990,  996,  996,  996, 1184,  996,  996,  996,  996,  996,
      996,  996,  996, 1000, 1000, 1000, 1184, 1000, 1000, 1000,
     1000, 1000, 1000, 1000, 1000,  982,  982,  982, 1184,  982,
      982,  982,  982,  982,  982,  982,  982,  994,  994,  994,

     1184,  994,  994,  994,  994,  994,  994,  994,  994,  998,
      998,  998, 1184,  998,  998,  998,  998,  998,  998,  998,
      998, 1051, 1051, 1051, 1184, 1051, 1051, 1051, 1051, 1051,
     1051, 1051, 1051, 1069, 1069, 1069, 1184, 1069, 1069, 1069,
     1069, 1069, 1069, 1069, 1069, 1049, 1049, 1049, 1184, 1049,
     1049, 1049, 1049, 1049, 1049, 1049, 1049, 1067, 1067, 1067,
     1184, 1067, 1067, 1067, 1067, 1067, 1067, 1067, 1067, 1098,
     1098, 1098, 1184, 1098, 1098, 1098, 1098, 1098, 1098, 1098,
     1098, 1103, 1103, 1103, 1184, 1103, 1103, 1103, 1103, 1103,
     1103, 1103, 1103, 1106, 1106, 1106, 1184, 1106, 1106, 1106,

     1106, 1106, 1106, 1106, 1106, 1109, 1109, 1109, 1184, 1109,
     1109, 1109, 1109, 1109, 1109, 1109, 1109, 1118, 1118, 1118,
     1184, 1118, 1118, 1118, 1118, 1118, 1118, 1118, 1118, 1096,
     1096, 1096, 1184, 1096, 1096, 1096, 1096, 1096, 1096, 1096,
     1096, 1101, 1101, 1101, 1184, 1101, 1101, 1101, 1101, 1101,
     1101, 1101, 1101, 1104, 1104, 1104, 1184, 1104, 1104, 1104,
     1104, 1104, 1104, 1104, 1104, 1107, 1107, 1107, 1184, 1107,
     1107, 1107, 1107, 1107, 1107, 1107, 1107, 1131, 1131, 1131,
     1184, 1131, 1131, 1131, 1131, 1131, 1131, 1131, 1131, 1116,
     1116, 1116, 1184, 1116, 1116, 1116, 1116, 1116, 1116, 1116,

     1116, 1139, 1139, 1139, 1184, 1139, 1139, 1139, 1139, 1139,
     1139, 1139, 1139, 1144, 1144, 1144, 1184, 1144, 1144, 1144,
     1144, 1144, 1144, 1144, 1144, 1129, 1129, 1129, 1184, 1129,
     1129, 1129, 1129, 1129, 1129, 1129, 1129, 1148, 1148, 1148,
     1184, 1148, 1148, 1148, 1148, 1148, 1148, 1148, 1148, 1152,
     1152, 1152, 1184, 1152, 1152, 1152, 1152, 1152, 1152, 1152,
     1152, 1137, 1137, 1137, 1184, 1137, 1137, 1137, 1137, 1137,
     1137, 1137, 1137, 1158, 1158, 1158, 1184, 1158, 1158, 1158,
     1158, 1158, 1158, 1158, 1158, 1142, 1142, 1142, 1184, 1142,
     1142, 1142, 1142, 1142, 1142, 1142, 1142, 1146, 1146, 1146,

     1184, 1146, 1146, 1146, 1146, 1146, 1146, 1146, 1146, 1150,
     1150, 1150, 1184, 1150, 1150, 1150, 1150, 1150, 1150, 1150,
     1150, 1165, 1165, 1165, 1184, 1165, 1165, 1165, 1165, 1165,
     1165, 1165, 1165, 1156, 1156, 1156, 1184, 1156, 1156, 1156,
     1156, 1156, 1156, 1156, 1156, 1168, 1168, 1168, 1184, 1168,
     1168, 1168, 1168, 1168, 1168, 1168, 1168, 1163, 1163, 1163,
     1184, 1163, 1163, 1163, 1163, 1163, 1163, 1163, 1163, 1166,
     1166, 1166, 1184, 1166, 1166, 1166, 1166, 1166, 1166, 1166,
     1166, 1174, 1174, 1174, 1184, 1174, 1174, 1174, 1174, 1174,
     1174, 1174, 1174, 1172, 1172, 1172, 1184, 1172, 1172, 1172,

     1172, 1172, 1172, 1172, 1172, 1180, 1180, 1180, 1184, 1180,
     1180, 1180, 1180, 1180, 1180, 1180, 1180, 1183, 1183, 1183,
     1184, 1183, 1183, 1183, 1183, 1183, 1183, 1183, 1183, 1178,
     1178, 1178, 1184, 1178, 1178, 1178, 1178, 1178, 1178, 1178,
     1178, 1181, 1181, 1181, 1184, 1181, 1181, 1181, 1181, 1181,
     1181, 1181, 1181,   15, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,

     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184
    } ;

static yyconst flex_int16_t yy_chk[6026] =
    {   0,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
        1,    3,    4,    5,    6,    9,   10,   26,    9,   10,
       77,    3,    4,    5,    6,   22,   30,   22, 1153,   26,
       30,   22,   77,   22,   28,   22,   22,   28,   22,   32,

       32,   22,   22,   40,   22,   29,   29,   44,   29,   33,
       33, 1154,    9,   10,   11,   41,   11,   48,   11,   34,
       34,   11,   11,   24,   24,   42,   39,   43,   24,   11,
       24,   11,   24,   78,   11,   40,   78,   11,   46,   11,
       24,   11,   24,   41,   45,  100,   24,   41,   11,   44,
       40,  100,   11,   24,   44,   41,   24,   42,   43,   48,
       11,   42,   11,   11,   47,   39,   43,   39,   43,   11,
       31,   39,   31,   39,   46,   43,   45,   38,   31,   38,
       38,   38,   58,   49,   31,   38,   31,   31,   31,   31,
       38,   52,   38,   38,   50,   38,   38,   47,   31,   51,

       53,   62,   54, 1160,  108,   47,  108,  109,   57,  109,
       31,   49,  117,  117,   58,  117,   31,  226,   56,   49,
       55,  523,   50,   59,  523,   49,   50,   51,   53,   60,
      226,   51,   54,   62,   54,   52,   50,   86,   52,   51,
       53,   51,   53,   54,   53,   54,   56,   53,   54,   57,
       55,   59,   87,   57,   56,   59,   55,  149,   59,   59,
       55,   62,   56,   55,   88,   59,   55,   55,   90,   86,
       60,   60,   63,   76,   63,   63,   63,   89,   76,   76,
       63,   91,  147,   63,   86,   63,   76,   63,   63,  141,
       63,   63,   88,  121,   87,  121,   88,  141,   90,   87,

       90,  121,  149,  137,   88,   89,   88,  154,  148,   90,
      336,   90,  150,  106,   90,  106,  160,   89,  151,   89,
      137,   89,   91,   91,   89,  103,   76,  103,  152,  106,
      106,  147,  103,  103,  163,  153,  157,  106,  107,  148,
      107,  103,  123,  123,  123,  123,  154,  160,  336,  106,
      106,  155,  151,  106,  107,  107,  107,  107,  150,  110,
      110,  152,  159,  158,  110,  162,  110,  125,  125,  161,
      156,  125,  125,  125,  125,  163,  110,  106,  110,  106,
      153,  157,  110,  122,  122,  140,  164,  140,  122,  110,
      122,  165,  122,  155,  166,  168,  140,  158,  156,  169,

      122,  161,  122,  156,  167,  173,  122,  159,  170,  162,
      175,  171,  172,  122,  174,  156,  122,  177,  176,  178,
      180,  184,  182,  181,  183,  165,  191,  164,  192,  185,
      186,  168,  190,  188,  167,  166,  173,  170,  195,  189,
      167,  171,  172,  169,  187,  174,  167,  167,  194,  167,
      167,  175,  176,  193,  191,  196,  197,  184,  199,  177,
      198,  200,  192,  178,  182,  178,  180,  181,  185,  183,
      186,  201,  187,  188,  202,  203,  190,  204,  187,  189,
      194,  193,  195,  187,  205,  197,  199,  187,  206,  207,
      218,  209,  208,  219,  199,  221,  221,  272,  196,  233,

      198,  197,  272,  201,  202,  204,  200,  220,  220,  230,
      230,  203,  239,  233,  239,  220,  203, 1161,  240,  240,
      205,  264,  242,  242,  242,  242,  240,  264,  239,  239,
      274,  274,  206,  207,  208,  209,  239,  218,  240,  240,
      278,  219,  240,  241,  277,  241,  242,  279,  239,  239,
      291,  243,  239,  243,  244,  220,  244,  275,  280,  241,
      241,  241,  241,  245,  283,  245,  240,  281,  240,  246,
      246,  246,  250,  251,  250,  251,  239,  246,  239,  249,
      277,  249,  278,  279,  248,  275,  248,  285,  282,  246,
      246,  291,  293,  246,  280,  249,  249,  249,  249,  281,

      248,  248,  253,  253,  253,  253,  252,  252,  248,  283,
      284,  252,  270,  252,  253,  285,  270,  246,  286,  246,
      248,  248,  293,  252,  248,  252,  253,  282,  270,  252,
      285,  270,  253,  287,  288,  289,  252,  254,  254,  254,
      254,  255,  255,  255,  255,  290,  298,  295,  248,  299,
      248,  292,  294,  296,  286,  284,  300,  303,  301,  306,
      304,  287,  297,  308,  307,  255,  288,  289,  305,  309,
      302,  315,  312,  317,  310,  290,  320,  311,  290,  290,
      319,  292,  295,  292,  294,  290,  314,  306,  290,  292,
      290,  290,  298,  297,  304,  296,  299,  316,  303,  308,

      294,  297,  302,  300,  301,  310,  318,  305,  307,  311,
      321,  322,  305,  324,  315,  320,  309,  312,  317,  325,
      319,  326,  314,  327,  334,  316,  328,  329,  330,  331,
      332,  333,  335,  338,  339,  342,  339,  339,  318,  340,
      337,  343,  345,  346,  349,  324,  344,  347, 1162,  413,
      322,  325,  321,  385,  328,  385,  372,  386, 1170,  386,
      332, 1171,  339,  326,  415,  327,  329,  334,  331,  330,
      372,  335,  337,  333,  338,  340,  339,  344,  339,  347,
      345,  342,  346,  380,  380,  380,  380,  343,  349,  381,
      387,  381,  387,  396,  382,  382,  384,  384,  384,  384,

      413,  396,  382,  399,  415,  381,  381,  380,  416,  399,
      417,  423,  425,  381,  382,  382,  399,  383,  382,  383,
      384,  421,  414, 1175,  418,  381,  381,  424,  430,  381,
      388,  388,  388,  383,  383,  383,  383,  422,  388,  423,
      428,  425,  382,  417,  382,  389,  389,  389,  389,  416,
      388,  388,  429,  381,  388,  381,  414,  389,  390,  390,
      430,  424,  390,  390,  390,  390,  421,  431,  428,  389,
      391,  391,  391,  391,  418,  389,  418,  432,  388,  433,
      388,  435,  428,  434,  422,  436,  437,  439,  440,  442,
      441,  445,  444,  446,  391,  443,  447,  448,  449,  429,

      457,  450,  454,  453,  456,  452,  455,  462,  463,  431,
      458,  434,  464,  460,  469,  467,  470,  436,  441,  466,
      471,  432,  433,  473,  474,  472,  449,  435,  444,  446,
      454,  475,  440,  439,  437,  450,  443,  452,  445,  453,
      462,  442,  447,  448,  460,  455,  456,  457,  471,  458,
      466,  467,  464,  472,  468,  463,  468,  468,  476,  469,
      477,  478,  470,  479,  480,  481,  482,  485,  473,  474,
      488,  489,  475,  554,  491,  490,  492,  495,  496,  495,
      495,  528,  528,  528,  528,  468,  476, 1149,  527,  527,
      527,  527,  477,  529,  529,  529,  529,  563,  482,  479,

      478,  468,  553,  485,  481,  480,  479,  490,  491,  555,
      496,  488,  527,  562,  554,  489,  560,  529,  492,  536,
      536,  536,  561,  536,  536,  536,  536,  564,  536,  536,
      536,  536,  536,  536,  536,  536,  536,  555,  563,  565,
      566,  536,  536,  536,  536,  536,  557,  553,  557,  557,
      567,  568,  560,  572,  569,  571,  586,  562,  570,  536,
      536,  561,  536,  575,  576,  577,  564,  578,  579,  580,
      579,  579,  581,  582,  583,  587,  589,  591,  590,  592,
      594,  566,  572,  593,  565,  569,  597,  571,  536,  536,
      595,  567,  568,  577,  570,  575,  598,  586,  572,  599,

      576,  580,  583,  596,  582,  596,  596,  587,  600,  603,
      604,  578,  594,  606,  607,  608,  581,  613,  616,  592,
      591,  589,  590,  593,  595,  612,  611,  615,  598,  617,
      618,  620,  597,  599,  621,  624,  623,  677,  625,  679,
      674,  681,  646,  646,  646,  646,  616,  682,  613,  606,
      684,  603,  689,  604,  600,  654,  654,  678,  654,  607,
      615,  608,  611,  617,  620,  621,  646,  679,  683,  612,
      623,  677,  685,  681,  686,  687,  624,  692,  690,  682,
      684,  618,  625,  655,  655,  655,  674,  655,  655,  655,
      655,  689,  655,  655,  655,  655,  655,  655,  655,  655,

      655,  683,  688,  678,  685,  655,  655,  655,  655,  655,
      699,  700,  705,  691,  701,  687,  690,  709,  702,  692,
      798,  686, 1145,  655,  655,  738,  655,  693,  698,  703,
      704,  726,  688,  726,  726, 1140,  713,  716,  724,  847,
      705,  706,  699,  738,  701,  718,  847,  725,  715,  709,
      700,  798,  655,  655,  657,  657,  657,  691,  657,  657,
      657,  657,  702,  657,  657,  657,  657,  657,  657,  657,
      657,  657,  714,  693,  698,  703,  657,  657,  657,  657,
      657,  704,  713,  706,  715,  718,  716,  719,  721,  724,
      722,  725,  733, 1136,  657,  657,  727,  657,  727,  727,

      728,  731,  728,  728,  714,  776,  733,  733,  733,  733,
      773,  732,  753,  753,  780,  753,  774,  758,  758,  719,
      758,  721,  722,  657,  657,  660,  660,  660,  779,  660,
      660,  660,  660,  731,  660,  660,  660,  660,  660,  660,
      660,  660,  660,  732,  781,  776,  773,  660,  660,  660,
      660,  660,  771,  771,  782,  771,  783,  774,  785,  789,
      780,  809, 1135,  784,  779,  660,  660, 1133,  660,  802,
     1128,  797,  793,  788,  806, 1126,  806,  806,  835,  835,
      884,  835,  803,  876,  800,  876,  884,  858,  781,  809,
      785,  789,  783,  782,  660,  660,  671,  671,  671,  784,

      671,  671,  671,  671,  793,  671,  671,  671,  671,  671,
      671,  671,  671,  671,  788,  797,  800,  802,  671,  671,
      671,  671,  671,  803,  804,  851,  807,  816,  817,  818,
      854,  857,  856,  858,  855, 1125,  671,  671, 1123,  671,
      865, 1122,  859,  818,  818,  818,  818,  846,  846, 1121,
      846,  866, 1120, 1119,  867,  851,  804,  807,  816,  817,
      856,  873,  910,  857,  854,  671,  671,  749,  749,  749,
      855,  749,  749,  749,  749,  859,  749,  749,  749,  749,
      749,  749,  749,  749,  749,  864,  867,  865,  889,  749,
      749,  749,  749,  749,  846,  866,  895,  895,  889,  895,

      911,  873,  941,  889,  910,  913,  914,  749,  749,  941,
      749,  901,  901,  864,  901, 1114,  915,  943,  943,  916,
      943,  947,  947,  953,  947,  951, 1113,  952,  972,  972,
     1112,  972, 1111,  956, 1110, 1100,  749,  749,  754,  754,
      754,  913,  754,  754,  754,  754,  911,  754,  754,  754,
      754,  754,  754,  754,  754,  754,  915,  914,  916,  952,
      754,  754,  754,  754,  754,  956,  951,  979,  979,  988,
      979,  953, 1004,  983,  983,  972,  983, 1099,  754,  754,
     1095,  754,  995,  995, 1004,  995,  999,  999, 1004,  999,
     1094, 1006, 1006, 1004, 1006, 1028, 1028, 1093, 1028, 1047,

     1047,  988, 1047, 1050, 1050, 1091, 1050,  754,  754,  759,
      759,  759, 1090,  759,  759,  759,  759, 1089,  759,  759,
      759,  759,  759,  759,  759,  759,  759, 1068, 1068, 1088,
     1068,  759,  759,  759,  759,  759, 1097, 1097, 1087, 1097,
     1102, 1102, 1086, 1102, 1105, 1105, 1082, 1105, 1081,  759,
      759, 1079,  759, 1108, 1108, 1078, 1108, 1117, 1117, 1077,
     1117, 1130, 1130, 1076, 1130, 1138, 1138, 1075, 1138, 1143,
     1143, 1074, 1143, 1147, 1147, 1073, 1147, 1072,  759,  759,
      772,  772,  772, 1071,  772,  772,  772,  772, 1070,  772,
      772,  772,  772,  772,  772,  772,  772,  772, 1151, 1151,

     1066, 1151,  772,  772,  772,  772,  772, 1157, 1157, 1065,
     1157, 1164, 1164, 1064, 1164, 1167, 1167, 1063, 1167, 1062,
      772,  772, 1061,  772, 1173, 1173, 1060, 1173, 1179, 1179,
     1059, 1179, 1182, 1182, 1058, 1182, 1057, 1056, 1055, 1054,
     1053, 1052, 1048, 1045, 1044, 1043, 1042, 1041, 1040,  772,
      772,  836,  836,  836, 1039,  836,  836,  836,  836, 1038,
      836,  836,  836,  836,  836,  836,  836,  836,  836, 1037,
     1036, 1035, 1034,  836,  836,  836,  836,  836, 1033, 1032,
     1031, 1029, 1027, 1026, 1025, 1024, 1023, 1022, 1021, 1020,
     1019,  836,  836, 1017,  836, 1016, 1015, 1014, 1013, 1012,

     1011, 1010, 1009, 1008, 1007, 1005, 1003, 1002, 1001,  997,
      991,  989,  987,  986,  985,  981,  980,  978,  977,  976,
      836,  836,  841,  841,  841,  975,  841,  841,  841,  841,
      974,  841,  841,  841,  841,  841,  841,  841,  841,  841,
      973,  971,  970,  969,  841,  841,  841,  841,  841,  968,
      967,  965,  962,  961,  959,  957,  955,  954,  950,  945,
      944,  942,  841,  841,  940,  841,  939,  938,  937,  936,
      935,  934,  933,  932,  931,  930,  929,  926,  925,  924,
      923,  922,  920,  919,  918,  917,  912,  909,  908,  907,
      906,  841,  841,  896,  896,  896,  904,  896,  896,  896,

      896,  903,  896,  896,  896,  896,  896,  896,  896,  896,
      896,  902,  900,  899,  898,  896,  896,  896,  896,  896,
      897,  893,  892,  891,  890,  888,  887,  886,  882,  881,
      880,  879,  878,  896,  896,  877,  896,  875,  874,  872,
      871,  870,  869,  868,  863,  862,  861,  860,  853,  852,
      850,  849,  848,  845,  844,  843,  842,  840,  839,  838,
      837,  833,  896,  896,  905,  905,  905,  832,  905,  905,
      905,  905,  831,  905,  905,  905,  905,  905,  905,  905,
      905,  905,  830,  829,  828,  827,  905,  905,  905,  905,
      905,  826,  824,  823,  821,  820,  819,  815,  814,  813,

      812,  811,  810,  808,  905,  905,  805,  905,  801,  799,
      796,  795,  794,  792,  791,  790,  787,  786,  778,  777,
      775,  769,  768,  767,  766,  765,  764,  763,  762,  761,
      760,  756,  755,  905,  905,  948,  948,  948,  751,  948,
      948,  948,  948,  750,  948,  948,  948,  948,  948,  948,
      948,  948,  948,  748,  747,  746,  745,  948,  948,  948,
      948,  948,  744,  743,  741,  740,  739,  737,  736,  735,
      734,  730,  729,  723,  720,  948,  948,  717,  948,  712,
      711,  710,  708,  707,  697,  696,  695,  694,  680,  676,
      675,  673,  672,  670,  669,  668,  667,  666,  665,  664,

      663,  662,  661,  659,  948,  948,  949,  949,  949,  658,
      949,  949,  949,  949,  656,  949,  949,  949,  949,  949,
      949,  949,  949,  949,  652,  651,  650,  649,  949,  949,
      949,  949,  949,  648,  647,  645,  644,  643,  642,  641,
      638,  637,  633,  632,  631,  630,  949,  949,  629,  949,
      628,  627,  626,  622,  619,  614,  610,  609,  605,  602,
      601,  588,  585,  584,  574,  573,  559,  558,  556,  552,
      551,  550,  549,  548,  547,  949,  949,  964,  964,  964,
      546,  964,  964,  964,  964,  545,  964,  964,  964,  964,
      964,  964,  964,  964,  964,  544,  543,  542,  541,  964,

      964,  964,  964,  964,  540,  539,  538,  537,  535,  534,
      533,  532,  531,  530,  526,  525,  524,  964,  964,  522,
      964,  521,  519,  518,  517,  516,  514,  513,  512,  511,
      510,  499,  498,  497,  494,  493,  487,  486,  484,  483,
      465,  461,  459,  451,  438,  427,  964,  964,  966,  966,
      966,  426,  966,  966,  966,  966,  420,  966,  966,  966,
      966,  966,  966,  966,  966,  966,  419,  412,  411,  410,
      966,  966,  966,  966,  966,  409,  408,  407,  406,  405,
      404,  403,  402,  401,  400,  398,  397,  395,  966,  966,
      394,  966,  393,  392,  379,  378,  377,  376,  375,  374,

      373,  371,  370,  368,  366,  365,  364,  363,  362,  361,
      360,  359,  358,  357,  356,  355,  354,  966,  966,  984,
      984,  984,  353,  984,  984,  984,  984,  352,  984,  984,
      984,  984,  984,  984,  984,  984,  984,  351,  350,  348,
      341,  984,  984,  984,  984,  984,  323,  313,  276,  273,
      271,  269,  268,  267,  266,  265,  263,  262,  261,  984,
      984,  259,  984,  237,  236,  235,  234,  232,  231,  229,
      228,  227,  225,  224,  223,  214,  179,  146,  145,  144,
      143,  142,  139,  138,  132,  131,  128,  126,  984,  984,
      996,  996,  996,  105,  996,  996,  996,  996,  104,  996,

      996,  996,  996,  996,  996,  996,  996,  996,  102,  101,
       99,   98,  996,  996,  996,  996,  996,   97,   96,   95,
       93,   85,   84,   83,   82,   81,   75,   69,   66,   61,
      996,  996,   37,  996,   35,   27,   25,   23,   19,   15,
       12,    8,    7,    2,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,  996,
      996, 1000, 1000, 1000,    0, 1000, 1000, 1000, 1000,    0,
     1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000, 1000,    0,
        0,    0,    0, 1000, 1000, 1000, 1000, 1000,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,

        0, 1000, 1000,    0, 1000,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
     1000, 1000, 1030, 1030, 1030,    0, 1030, 1030, 1030, 1030,
        0, 1030, 1030, 1030, 1030, 1030, 1030, 1030, 1030, 1030,
        0,    0,    0,    0, 1030, 1030, 1030, 1030, 1030,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0, 1030, 1030,    0, 1030,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,

        0, 1030, 1030, 1046, 1046, 1046,    0, 1046, 1046, 1046,
     1046,    0, 1046, 1046, 1046, 1046, 1046, 1046, 1046, 1046,
     1046,    0,    0,    0,    0, 1046, 1046, 1046, 1046, 1046,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0, 1046, 1046,    0, 1046,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0, 1046, 1046, 1051, 1051, 1051,    0, 1051, 1051,
     1051, 1051,    0, 1051, 1051, 1051, 1051, 1051, 1051, 1051,
     1051, 1051,    0,    0,    0,    0, 1051, 1051, 1051, 1051,

     1051,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0, 1051, 1051,    0, 1051,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0, 1051, 1051, 1069, 1069, 1069,    0, 1069,
     1069, 1069, 1069,    0, 1069, 1069, 1069, 1069, 1069, 1069,
     1069, 1069, 1069,    0,    0,    0,    0, 1069, 1069, 1069,
     1069, 1069,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0, 1069, 1069,    0, 1069,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,

        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0, 1069, 1069, 1080, 1080, 1080,    0,
     1080, 1080, 1080, 1080,    0, 1080, 1080, 1080, 1080, 1080,
     1080, 1080, 1080, 1080,    0,    0,    0,    0, 1080, 1080,
     1080, 1080, 1080,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0, 1080, 1080,    0, 1080,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0, 1080, 1080, 1083, 1083, 1083,
        0, 1083, 1083, 1083, 1083,    0, 1083, 1083, 1083, 1083,

     1083, 1083, 1083, 1083, 1083,    0,    0,    0,    0, 1083,
     1083, 1083, 1083, 1083,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0, 1083, 1083,    0,
     1083,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0, 1083, 1083, 1084, 1084,
     1084,    0, 1084, 1084, 1084, 1084,    0, 1084, 1084, 1084,
     1084, 1084, 1084, 1084, 1084, 1084,    0,    0,    0,    0,
     1084, 1084, 1084, 1084, 1084,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0, 1084, 1084,

        0, 1084,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0, 1084, 1084, 1085,
     1085, 1085,    0, 1085, 1085, 1085, 1085,    0, 1085, 1085,
     1085, 1085, 1085, 1085, 1085, 1085, 1085,    0,    0,    0,
        0, 1085, 1085, 1085, 1085, 1085,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0, 1085,
     1085,    0, 1085,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0, 1085, 1085,

     1092, 1092, 1092,    0, 1092, 1092, 1092, 1092,    0, 1092,
     1092, 1092, 1092, 1092, 1092, 1092, 1092, 1092,    0,    0,
        0,    0, 1092, 1092, 1092, 1092, 1092,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
     1092, 1092,    0, 1092,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0, 1092,
     1092, 1098, 1098, 1098,    0, 1098, 1098, 1098, 1098,    0,
     1098, 1098, 1098, 1098, 1098, 1098, 1098, 1098, 1098,    0,
        0,    0,    0, 1098, 1098, 1098, 1098, 1098,    0,    0,

        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0, 1098, 1098,    0, 1098,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
     1098, 1098, 1103, 1103, 1103,    0, 1103, 1103, 1103, 1103,
        0, 1103, 1103, 1103, 1103, 1103, 1103, 1103, 1103, 1103,
        0,    0,    0,    0, 1103, 1103, 1103, 1103, 1103,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0, 1103, 1103,    0, 1103,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,

        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0, 1103, 1103, 1106, 1106, 1106,    0, 1106, 1106, 1106,
     1106,    0, 1106, 1106, 1106, 1106, 1106, 1106, 1106, 1106,
     1106,    0,    0,    0,    0, 1106, 1106, 1106, 1106, 1106,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0, 1106, 1106,    0, 1106,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0, 1106, 1106, 1109, 1109, 1109,    0, 1109, 1109,
     1109, 1109,    0, 1109, 1109, 1109, 1109, 1109, 1109, 1109,

     1109, 1109,    0,    0,    0,    0, 1109, 1109, 1109, 1109,
     1109,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0, 1109, 1109,    0, 1109,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0, 1109, 1109, 1115, 1115, 1115,    0, 1115,
     1115, 1115, 1115,    0, 1115, 1115, 1115, 1115, 1115, 1115,
     1115, 1115, 1115,    0,    0,    0,    0, 1115, 1115, 1115,
     1115, 1115,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0, 1115, 1115,    0, 1115,    0,

        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0, 1115, 1115, 1118, 1118, 1118,    0,
     1118, 1118, 1118, 1118,    0, 1118, 1118, 1118, 1118, 1118,
     1118, 1118, 1118, 1118,    0,    0,    0,    0, 1118, 1118,
     1118, 1118, 1118,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0, 1118, 1118,    0, 1118,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0, 1118, 1118, 1124, 1124, 1124,

        0, 1124, 1124, 1124, 1124,    0, 1124, 1124, 1124, 1124,
     1124, 1124, 1124, 1124, 1124,    0,    0,    0,    0, 1124,
     1124, 1124, 1124, 1124,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0, 1124, 1124,    0,
     1124,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0, 1124, 1124, 1127, 1127,
     1127,    0, 1127, 1127, 1127, 1127,    0, 1127, 1127, 1127,
     1127, 1127, 1127, 1127, 1127, 1127,    0,    0,    0,    0,
     1127, 1127, 1127, 1127, 1127,    0,    0,    0,    0,    0,

        0,    0,    0,    0,    0,    0,    0,    0, 1127, 1127,
        0, 1127,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0, 1127, 1127, 1131,
     1131, 1131,    0, 1131, 1131, 1131, 1131,    0, 1131, 1131,
     1131, 1131, 1131, 1131, 1131, 1131, 1131,    0,    0,    0,
        0, 1131, 1131, 1131, 1131, 1131,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0, 1131,
     1131,    0, 1131,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,

        0,    0,    0,    0,    0,    0,    0,    0, 1131, 1131,
     1132, 1132, 1132,    0, 1132, 1132, 1132, 1132,    0, 1132,
     1132, 1132, 1132, 1132, 1132, 1132, 1132, 1132,    0,    0,
        0,    0, 1132, 1132, 1132, 1132, 1132,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
     1132, 1132,    0, 1132,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0, 1132,
     1132, 1134, 1134, 1134,    0, 1134, 1134, 1134, 1134,    0,
     1134, 1134, 1134, 1134, 1134, 1134, 1134, 1134, 1134,    0,

        0,    0,    0, 1134, 1134, 1134, 1134, 1134,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0, 1134, 1134,    0, 1134,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
     1134, 1134, 1139, 1139, 1139,    0, 1139, 1139, 1139, 1139,
        0, 1139, 1139, 1139, 1139, 1139, 1139, 1139, 1139, 1139,
        0,    0,    0,    0, 1139, 1139, 1139, 1139, 1139,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0, 1139, 1139,    0, 1139,    0,    0,    0,    0,

        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0, 1139, 1139, 1141, 1141, 1141,    0, 1141, 1141, 1141,
     1141,    0, 1141, 1141, 1141, 1141, 1141, 1141, 1141, 1141,
     1141,    0,    0,    0,    0, 1141, 1141, 1141, 1141, 1141,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0, 1141, 1141,    0, 1141,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0, 1141, 1141, 1144, 1144, 1144,    0, 1144, 1144,

     1144, 1144,    0, 1144, 1144, 1144, 1144, 1144, 1144, 1144,
     1144, 1144,    0,    0,    0,    0, 1144, 1144, 1144, 1144,
     1144,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0, 1144, 1144,    0, 1144,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0, 1144, 1144, 1148, 1148, 1148,    0, 1148,
     1148, 1148, 1148,    0, 1148, 1148, 1148, 1148, 1148, 1148,
     1148, 1148, 1148,    0,    0,    0,    0, 1148, 1148, 1148,
     1148, 1148,    0,    0,    0,    0,    0,    0,    0,    0,

        0,    0,    0,    0,    0, 1148, 1148,    0, 1148,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0, 1148, 1148, 1152, 1152, 1152,    0,
     1152, 1152, 1152, 1152,    0, 1152, 1152, 1152, 1152, 1152,
     1152, 1152, 1152, 1152,    0,    0,    0,    0, 1152, 1152,
     1152, 1152, 1152,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0, 1152, 1152,    0, 1152,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,

        0,    0,    0,    0,    0, 1152, 1152, 1155, 1155, 1155,
        0, 1155, 1155, 1155, 1155,    0, 1155, 1155, 1155, 1155,
     1155, 1155, 1155, 1155, 1155,    0,    0,    0,    0, 1155,
     1155, 1155, 1155, 1155,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0, 1155, 1155,    0,
     1155,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0, 1155, 1155, 1158, 1158,
     1158,    0, 1158, 1158, 1158, 1158,    0, 1158, 1158, 1158,
     1158, 1158, 1158, 1158, 1158, 1158,    0,    0,    0,    0,

     1158, 1158, 1158, 1158, 1158,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0, 1158, 1158,
        0, 1158,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0, 1158, 1158, 1159,
     1159, 1159,    0, 1159, 1159, 1159, 1159,    0, 1159, 1159,
     1159, 1159, 1159, 1159, 1159, 1159, 1159,    0,    0,    0,
        0, 1159, 1159, 1159, 1159, 1159,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0, 1159,
     1159,    0, 1159,    0,    0,    0,    0,    0,    0,    0,

        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0, 1159, 1159,
     1165, 1165, 1165,    0, 1165, 1165, 1165, 1165,    0, 1165,
     1165, 1165, 1165, 1165, 1165, 1165, 1165, 1165,    0,    0,
        0,    0, 1165, 1165, 1165, 1165, 1165,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
     1165, 1165,    0, 1165,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0, 1165,
     1165, 1168, 1168, 1168,    0, 1168, 1168, 1168, 1168,    0,

     1168, 1168, 1168, 1168, 1168, 1168, 1168, 1168, 1168,    0,
        0,    0,    0, 1168, 1168, 1168, 1168, 1168,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0, 1168, 1168,    0, 1168,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
     1168, 1168, 1169, 1169, 1169,    0, 1169, 1169, 1169, 1169,
        0, 1169, 1169, 1169, 1169, 1169, 1169, 1169, 1169, 1169,
        0,    0,    0,    0, 1169, 1169, 1169, 1169, 1169,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,

        0,    0, 1169, 1169,    0, 1169,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0, 1169, 1169, 1174, 1174, 1174,    0, 1174, 1174, 1174,
     1174,    0, 1174, 1174, 1174, 1174, 1174, 1174, 1174, 1174,
     1174,    0,    0,    0,    0, 1174, 1174, 1174, 1174, 1174,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0, 1174, 1174,    0, 1174,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,

        0,    0, 1174, 1174, 1176, 1176, 1176,    0, 1176, 1176,
     1176, 1176,    0, 1176, 1176, 1176, 1176, 1176, 1176, 1176,
     1176, 1176,    0,    0,    0,    0, 1176, 1176, 1176, 1176,
     1176,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0, 1176, 1176,    0, 1176,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0, 1176, 1176, 1177, 1177, 1177,    0, 1177,
     1177, 1177, 1177,    0, 1177, 1177, 1177, 1177, 1177, 1177,
     1177, 1177, 1177,    0,    0,    0,    0, 1177, 1177, 1177,

     1177, 1177,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0, 1177, 1177,    0, 1177,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0, 1177, 1177, 1180, 1180, 1180,    0,
     1180, 1180, 1180, 1180,    0, 1180, 1180, 1180, 1180, 1180,
     1180, 1180, 1180, 1180,    0,    0,    0,    0, 1180, 1180,
     1180, 1180, 1180,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0, 1180, 1180,    0, 1180,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,

        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0, 1180, 1180, 1183, 1183, 1183,
        0, 1183, 1183, 1183, 1183,    0, 1183, 1183, 1183, 1183,
     1183, 1183, 1183, 1183, 1183,    0,    0,    0,    0, 1183,
     1183, 1183, 1183, 1183,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0, 1183, 1183,    0,
     1183,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
        0,    0,    0,    0,    0,    0, 1183, 1183, 1185, 1185,
     1185, 1185, 1185, 1185, 1185, 1185, 1185, 1185, 1185, 1185,

     1186, 1186, 1186, 1186, 1186, 1186, 1186, 1186, 1186, 1186,
     1186, 1186, 1187, 1187, 1187, 1187, 1187, 1187, 1187, 1187,
     1187, 1187, 1187, 1187, 1188, 1188, 1188, 1188, 1188, 1188,
     1188, 1188, 1188, 1188, 1188, 1188, 1189, 1189, 1189, 1189,
     1189, 1189, 1189, 1189, 1189, 1189, 1189, 1189, 1190,    0,
     1190, 1190,    0, 1190, 1190, 1190, 1191, 1191,    0,    0,
        0,    0,    0,    0, 1191, 1191, 1191, 1192, 1192, 1192,
     1192,    0, 1192, 1192, 1192, 1193,    0,    0, 1193, 1193,
     1193, 1193, 1193, 1193, 1193, 1193, 1193, 1194, 1194, 1194,
     1195,    0,    0,    0, 1195, 1195, 1195, 1195,    0, 1195,

     1196,    0,    0,    0, 1196,    0, 1196,    0,    0, 1196,
     1197,    0, 1197, 1197,    0, 1197, 1197, 1197, 1198, 1198,
     1198, 1199,    0, 1199, 1199,    0, 1199, 1199, 1199, 1200,
        0,    0,    0, 1200, 1200, 1200, 1200,    0, 1200, 1201,
        0,    0,    0, 1201,    0, 1201,    0,    0, 1201, 1202,
     1202, 1202, 1202,    0, 1202, 1202, 1202, 1203, 1203, 1203,
        0, 1203, 1203, 1203, 1203, 1203, 1203, 1203, 1203, 1204,
     1204, 1204,    0, 1204, 1204, 1204, 1204, 1204, 1204, 1204,
     1204, 1205, 1205, 1205,    0, 1205, 1205, 1205, 1205, 1205,
     1205, 1205, 1205, 1206, 1206, 1206,    0, 1206, 1206, 1206,

     1206, 1206, 1206, 1206, 1206, 1207, 1207, 1207,    0, 1207,
     1207, 1207, 1207, 1207, 1207, 1207, 1207, 1208, 1208, 1208,
        0, 1208, 1208, 1208, 1208, 1208, 1208, 1208, 1208, 1209,
     1209, 1209,    0, 1209, 1209, 1209, 1209, 1209, 1209, 1209,
     1209, 1210, 1210, 1210,    0, 1210, 1210, 1210, 1210, 1210,
     1210, 1210, 1210, 1211, 1211, 1211,    0, 1211, 1211, 1211,
     1211, 1211, 1211, 1211, 1211, 1212, 1212, 1212,    0, 1212,
     1212, 1212, 1212, 1212, 1212, 1212, 1212, 1213, 1213, 1213,
        0, 1213, 1213, 1213, 1213, 1213, 1213, 1213, 1213, 1214,
     1214, 1214,    0, 1214, 1214, 1214, 1214, 1214, 1214, 1214,

     1214, 1215, 1215, 1215,    0, 1215, 1215, 1215, 1215, 1215,
     1215, 1215, 1215, 1216, 1216, 1216, 1216, 1216, 1216, 1216,
     1216, 1216, 1216, 1216, 1216, 1217, 1217, 1217,    0, 1217,
     1217, 1217, 1217, 1217, 1217, 1217, 1217, 1218, 1218, 1218,
        0, 1218, 1218, 1218, 1218, 1218, 1218, 1218, 1218, 1219,
     1219, 1219,    0, 1219, 1219, 1219, 1219, 1219, 1219, 1219,
     1219, 1220, 1220, 1220,    0, 1220, 1220, 1220, 1220, 1220,
     1220, 1220, 1220, 1221, 1221, 1221,    0, 1221, 1221, 1221,
     1221, 1221, 1221, 1221, 1221, 1222, 1222, 1222,    0, 1222,
     1222, 1222, 1222, 1222, 1222, 1222, 1222, 1223, 1223, 1223,

        0, 1223, 1223, 1223, 1223, 1223, 1223, 1223, 1223, 1224,
     1224, 1224,    0, 1224, 1224, 1224, 1224, 1224, 1224, 1224,
     1224, 1225, 1225, 1225,    0, 1225, 1225, 1225, 1225, 1225,
     1225, 1225, 1225, 1226, 1226, 1226,    0, 1226, 1226, 1226,
     1226, 1226, 1226, 1226, 1226, 1227, 1227, 1227,    0, 1227,
     1227, 1227, 1227, 1227, 1227, 1227, 1227, 1228, 1228, 1228,
        0, 1228, 1228, 1228, 1228, 1228, 1228, 1228, 1228, 1229,
     1229, 1229,    0, 1229, 1229, 1229, 1229, 1229, 1229, 1229,
     1229, 1230, 1230, 1230,    0, 1230, 1230, 1230, 1230, 1230,
     1230, 1230, 1230, 1231, 1231, 1231,    0, 1231, 1231, 1231,

     1231, 1231, 1231, 1231, 1231, 1232, 1232, 1232,    0, 1232,
     1232, 1232, 1232, 1232, 1232, 1232, 1232, 1233, 1233, 1233,
        0, 1233, 1233, 1233, 1233, 1233, 1233, 1233, 1233, 1234,
     1234, 1234,    0, 1234, 1234, 1234, 1234, 1234, 1234, 1234,
     1234, 1235, 1235, 1235,    0, 1235, 1235, 1235, 1235, 1235,
     1235, 1235, 1235, 1236, 1236, 1236,    0, 1236, 1236, 1236,
     1236, 1236, 1236, 1236, 1236, 1237, 1237, 1237,    0, 1237,
     1237, 1237, 1237, 1237, 1237, 1237, 1237, 1238, 1238, 1238,
        0, 1238, 1238, 1238, 1238, 1238, 1238, 1238, 1238, 1239,
     1239, 1239,    0, 1239, 1239, 1239, 1239, 1239, 1239, 1239,

     1239, 1240, 1240, 1240,    0, 1240, 1240, 1240, 1240, 1240,
     1240, 1240, 1240, 1241, 1241, 1241,    0, 1241, 1241, 1241,
     1241, 1241, 1241, 1241, 1241, 1242, 1242, 1242,    0, 1242,
     1242, 1242, 1242, 1242, 1242, 1242, 1242, 1243, 1243, 1243,
        0, 1243, 1243, 1243, 1243, 1243, 1243, 1243, 1243, 1244,
     1244, 1244,    0, 1244, 1244, 1244, 1244, 1244, 1244, 1244,
     1244, 1245, 1245, 1245,    0, 1245, 1245, 1245, 1245, 1245,
     1245, 1245, 1245, 1246, 1246, 1246,    0, 1246, 1246, 1246,
     1246, 1246, 1246, 1246, 1246, 1247, 1247, 1247,    0, 1247,
     1247, 1247, 1247, 1247, 1247, 1247, 1247, 1248, 1248, 1248,

        0, 1248, 1248, 1248, 1248, 1248, 1248, 1248, 1248, 1249,
     1249, 1249,    0, 1249, 1249, 1249, 1249, 1249, 1249, 1249,
     1249, 1250, 1250, 1250,    0, 1250, 1250, 1250, 1250, 1250,
     1250, 1250, 1250, 1251, 1251, 1251,    0, 1251, 1251, 1251,
     1251, 1251, 1251, 1251, 1251, 1252, 1252, 1252,    0, 1252,
     1252, 1252, 1252, 1252, 1252, 1252, 1252, 1253, 1253, 1253,
        0, 1253, 1253, 1253, 1253, 1253, 1253, 1253, 1253, 1254,
     1254, 1254,    0, 1254, 1254, 1254, 1254, 1254, 1254, 1254,
     1254, 1255, 1255, 1255,    0, 1255, 1255, 1255, 1255, 1255,
     1255, 1255, 1255, 1256, 1256, 1256,    0, 1256, 1256, 1256,

     1256, 1256, 1256, 1256, 1256, 1257, 1257, 1257,    0, 1257,
     1257, 1257, 1257, 1257, 1257, 1257, 1257, 1258, 1258, 1258,
        0, 1258, 1258, 1258, 1258, 1258, 1258, 1258, 1258, 1259,
     1259, 1259,    0, 1259, 1259, 1259, 1259, 1259, 1259, 1259,
     1259, 1260, 1260, 1260,    0, 1260, 1260, 1260, 1260, 1260,
     1260, 1260, 1260, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,

     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184, 1184,
     1184, 1184, 1184, 1184, 1184
    } ;

static yy_state_type yy_last_accepting_state;
static char *yy_last_accepting_cpos;

extern int verilog__flex_debug;
int verilog__flex_debug = 0;

/* The intent behind this definition is that it'll catch
 * any uses of REJECT which flex missed.
 */
#define REJECT reject_used_but_not_detected
static int yy_more_flag = 0;
static int yy_more_len = 0;
#define yymore() ((yy_more_flag) = 1)
#define YY_MORE_ADJ (yy_more_len)
#define YY_RESTORE_YY_MORE_OFFSET
char *verilog_text;
#line 1 "verilog.l"
#line 2 "verilog.l"
/*
 * Copyright (c) 1999-2003 moe
 *
 *    This source code is free software; you can redistribute it
 *    and/or modify it in source code form under the terms of the GNU
 *    General Public License as published by the Free Software
 *    Foundation; either version 2 of the License, or (at your option)
 *    any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include "parse.tab.h"
#include <cassert>

extern FILE*  verilog_input;
extern string verilog_file;
extern string verilog_comment;

unsigned int line;
string       file;

#define YY_USER_INIT reset_lexor();
#define yylval verilog_lval

static void reset_lexor();
static int comment_enter;

static void line_directive();







#line 2266 "_lexor.cc"

#define INITIAL 0
#define ATTRIBUTE 1
#define CCOMMENT 2
#define LCOMMENT 3
#define CSTRING 4
#define UDPTABLE 5
#define PPTIMESCALE 6

#ifndef YY_NO_UNISTD_H
/* Special case for "unistd.h", since it is non-ANSI. We include it way
 * down here because we want the user's section 1 to have been scanned first.
 * The user has a chance to override it with an option.
 */
#include <unistd.h>
#endif

#ifndef YY_EXTRA_TYPE
#define YY_EXTRA_TYPE void *
#endif

static int yy_init_globals (void );

/* Accessor methods to globals.
   These are made visible to non-reentrant scanners for convenience. */

int verilog_lex_destroy (void );

int verilog_get_debug (void );

void verilog_set_debug (int debug_flag  );

YY_EXTRA_TYPE verilog_get_extra (void );

void verilog_set_extra (YY_EXTRA_TYPE user_defined  );

FILE *verilog_get_in (void );

void verilog_set_in  (FILE * _in_str  );

FILE *verilog_get_out (void );

void verilog_set_out  (FILE * _out_str  );

yy_size_t verilog_get_leng (void );

char *verilog_get_text (void );

int verilog_get_lineno (void );

void verilog_set_lineno (int _line_number  );

/* Macros after this point can all be overridden by user definitions in
 * section 1.
 */

#ifndef YY_SKIP_YYWRAP
#ifdef __cplusplus
extern "C" int verilog_wrap (void );
#else
extern int verilog_wrap (void );
#endif
#endif

#ifndef YY_NO_UNPUT
    
    static void yyunput (int c,char *buf_ptr  );
    
#endif

#ifndef yytext_ptr
static void yy_flex_strncpy (char *,yyconst char *,int );
#endif

#ifdef YY_NEED_STRLEN
static int yy_flex_strlen (yyconst char * );
#endif

#ifndef YY_NO_INPUT

#ifdef __cplusplus
static int yyinput (void );
#else
static int input (void );
#endif

#endif

/* Amount of stuff to slurp up with each read. */
#ifndef YY_READ_BUF_SIZE
#ifdef __ia64__
/* On IA-64, the buffer size is 16k, not 8k */
#define YY_READ_BUF_SIZE 16384
#else
#define YY_READ_BUF_SIZE 8192
#endif /* __ia64__ */
#endif

/* Copy whatever the last rule matched to the standard output. */
#ifndef ECHO
/* This used to be an fputs(), but since the string might contain NUL's,
 * we now use fwrite().
 */
#define ECHO do { if (fwrite( verilog_text, verilog_leng, 1, verilog_out )) {} } while (0)
#endif

/* Gets input and stuffs it into "buf".  number of characters read, or YY_NULL,
 * is returned in "result".
 */
#ifndef YY_INPUT
#define YY_INPUT(buf,result,max_size) \
	if ( YY_CURRENT_BUFFER_LVALUE->yy_is_interactive ) \
		{ \
		int c = '*'; \
		size_t n; \
		for ( n = 0; n < max_size && \
			     (c = getc( verilog_in )) != EOF && c != '\n'; ++n ) \
			buf[n] = (char) c; \
		if ( c == '\n' ) \
			buf[n++] = (char) c; \
		if ( c == EOF && ferror( verilog_in ) ) \
			YY_FATAL_ERROR( "input in flex scanner failed" ); \
		result = n; \
		} \
	else \
		{ \
		errno=0; \
		while ( (result = fread(buf, 1, max_size, verilog_in))==0 && ferror(verilog_in)) \
			{ \
			if( errno != EINTR) \
				{ \
				YY_FATAL_ERROR( "input in flex scanner failed" ); \
				break; \
				} \
			errno=0; \
			clearerr(verilog_in); \
			} \
		}\
\

#endif

/* No semi-colon after return; correct usage is to write "yyterminate();" -
 * we don't want an extra ';' after the "return" because that will cause
 * some compilers to complain about unreachable statements.
 */
#ifndef yyterminate
#define yyterminate() return YY_NULL
#endif

/* Number of entries by which start-condition stack grows. */
#ifndef YY_START_STACK_INCR
#define YY_START_STACK_INCR 25
#endif

/* Report a fatal error. */
#ifndef YY_FATAL_ERROR
#define YY_FATAL_ERROR(msg) yy_fatal_error( msg )
#endif

/* end tables serialization structures and prototypes */

/* Default declaration of generated scanner - a define so the user can
 * easily add parameters.
 */
#ifndef YY_DECL
#define YY_DECL_IS_OURS 1

extern int verilog_lex (void);

#define YY_DECL int verilog_lex (void)
#endif /* !YY_DECL */

/* Code executed at the beginning of each rule, after verilog_text and verilog_leng
 * have been set up.
 */
#ifndef YY_USER_ACTION
#define YY_USER_ACTION
#endif

/* Code executed at the end of each rule. */
#ifndef YY_BREAK
#define YY_BREAK /*LINTED*/break;
#endif

#define YY_RULE_SETUP \
	if ( verilog_leng > 0 ) \
		YY_CURRENT_BUFFER_LVALUE->yy_at_bol = \
				(verilog_text[verilog_leng - 1] == '\n'); \
	YY_USER_ACTION

/** The main scanner function which does all the work.
 */
YY_DECL
{
	yy_state_type yy_current_state;
	char *yy_cp, *yy_bp;
	int yy_act;
    
	if ( !(yy_init) )
		{
		(yy_init) = 1;

#ifdef YY_USER_INIT
		YY_USER_INIT;
#endif

		if ( ! (yy_start) )
			(yy_start) = 1;	/* first start state */

		if ( ! verilog_in )
			verilog_in = stdin;

		if ( ! verilog_out )
			verilog_out = stdout;

		if ( ! YY_CURRENT_BUFFER ) {
			verilog_ensure_buffer_stack ();
			YY_CURRENT_BUFFER_LVALUE =
				verilog__create_buffer(verilog_in,YY_BUF_SIZE );
		}

		verilog__load_buffer_state( );
		}

	{
#line 50 "verilog.l"


#line 2496 "_lexor.cc"

	while ( /*CONSTCOND*/1 )		/* loops until end-of-file is reached */
		{
		(yy_more_len) = 0;
		if ( (yy_more_flag) )
			{
			(yy_more_len) = (yy_c_buf_p) - (yytext_ptr);
			(yy_more_flag) = 0;
			}
		yy_cp = (yy_c_buf_p);

		/* Support of verilog_text. */
		*yy_cp = (yy_hold_char);

		/* yy_bp points to the position in yy_ch_buf of the start of
		 * the current run.
		 */
		yy_bp = yy_cp;

		yy_current_state = (yy_start);
		yy_current_state += YY_AT_BOL();
yy_match:
		do
			{
			YY_CHAR yy_c = yy_ec[YY_SC_TO_UI(*yy_cp)] ;
			if ( yy_accept[yy_current_state] )
				{
				(yy_last_accepting_state) = yy_current_state;
				(yy_last_accepting_cpos) = yy_cp;
				}
			while ( yy_chk[yy_base[yy_current_state] + yy_c] != yy_current_state )
				{
				yy_current_state = (int) yy_def[yy_current_state];
				if ( yy_current_state >= 1185 )
					yy_c = yy_meta[(unsigned int) yy_c];
				}
			yy_current_state = yy_nxt[yy_base[yy_current_state] + (unsigned int) yy_c];
			++yy_cp;
			}
		while ( yy_base[yy_current_state] != 5954 );

yy_find_action:
		yy_act = yy_accept[yy_current_state];
		if ( yy_act == 0 )
			{ /* have to back up */
			yy_cp = (yy_last_accepting_cpos);
			yy_current_state = (yy_last_accepting_state);
			yy_act = yy_accept[yy_current_state];
			}

		YY_DO_BEFORE_ACTION;

do_action:	/* This label is used only to access EOF actions. */

		switch ( yy_act )
	{ /* beginning of action switch */
			case 0: /* must back up */
			/* undo the effects of YY_DO_BEFORE_ACTION */
			*yy_cp = (yy_hold_char);
			yy_cp = (yy_last_accepting_cpos);
			yy_current_state = (yy_last_accepting_state);
			goto yy_find_action;

case 1:
/* rule 1 can match eol */
YY_RULE_SETUP
#line 52 "verilog.l"
{ line_directive(); }
	YY_BREAK
case 2:
YY_RULE_SETUP
#line 55 "verilog.l"
{ ; }
	YY_BREAK
case 3:
/* rule 3 can match eol */
YY_RULE_SETUP
#line 56 "verilog.l"
{ line += 1; }
	YY_BREAK
case 4:
YY_RULE_SETUP
#line 58 "verilog.l"
{ comment_enter = YY_START; BEGIN(LCOMMENT); }
	YY_BREAK
case 5:
YY_RULE_SETUP
#line 59 "verilog.l"
{ yymore(); }
	YY_BREAK
case 6:
/* rule 6 can match eol */
YY_RULE_SETUP
#line 60 "verilog.l"
{ verilog_comment.assign(verilog_text,strlen(verilog_text)-1);
                 line += 1; BEGIN(comment_enter); }
	YY_BREAK
case 7:
YY_RULE_SETUP
#line 63 "verilog.l"
{ comment_enter = YY_START; BEGIN(CCOMMENT); }
	YY_BREAK
case 8:
YY_RULE_SETUP
#line 64 "verilog.l"
{ yymore(); }
	YY_BREAK
case 9:
/* rule 9 can match eol */
YY_RULE_SETUP
#line 65 "verilog.l"
{ line += 1; yymore(); }
	YY_BREAK
case 10:
YY_RULE_SETUP
#line 66 "verilog.l"
{ verilog_comment.assign(verilog_text,strlen(verilog_text)-2);
                 BEGIN(comment_enter); }
	YY_BREAK
case 11:
YY_RULE_SETUP
#line 69 "verilog.l"
{ comment_enter = YY_START; BEGIN(ATTRIBUTE); }
	YY_BREAK
case 12:
YY_RULE_SETUP
#line 70 "verilog.l"
{ yymore(); }
	YY_BREAK
case 13:
/* rule 13 can match eol */
YY_RULE_SETUP
#line 71 "verilog.l"
{ line += 1;yymore(); }
	YY_BREAK
case 14:
YY_RULE_SETUP
#line 72 "verilog.l"
{ verilog_comment.assign(verilog_text,strlen(verilog_text)-2);
                   BEGIN(comment_enter);
                   return K_ATCOMM; }
	YY_BREAK
case 15:
YY_RULE_SETUP
#line 77 "verilog.l"
{ return K_LS; }
	YY_BREAK
case 16:
YY_RULE_SETUP
#line 78 "verilog.l"
{ return K_RS; }
	YY_BREAK
case 17:
YY_RULE_SETUP
#line 79 "verilog.l"
{ return K_ALS; }
	YY_BREAK
case 18:
YY_RULE_SETUP
#line 80 "verilog.l"
{ return K_ARS; }
	YY_BREAK
case 19:
YY_RULE_SETUP
#line 81 "verilog.l"
{ return K_POW; }
	YY_BREAK
case 20:
YY_RULE_SETUP
#line 82 "verilog.l"
{ return K_LE; }
	YY_BREAK
case 21:
YY_RULE_SETUP
#line 83 "verilog.l"
{ return K_GE; }
	YY_BREAK
case 22:
YY_RULE_SETUP
#line 84 "verilog.l"
{ return K_EG; }
	YY_BREAK
case 23:
YY_RULE_SETUP
#line 85 "verilog.l"
{ return K_SG; }
	YY_BREAK
case 24:
YY_RULE_SETUP
#line 86 "verilog.l"
{ return K_EQ; }
	YY_BREAK
case 25:
YY_RULE_SETUP
#line 87 "verilog.l"
{ return K_NE; }
	YY_BREAK
case 26:
YY_RULE_SETUP
#line 88 "verilog.l"
{ return K_CEQ; }
	YY_BREAK
case 27:
YY_RULE_SETUP
#line 89 "verilog.l"
{ return K_CNE; }
	YY_BREAK
case 28:
YY_RULE_SETUP
#line 90 "verilog.l"
{ return K_LOR; }
	YY_BREAK
case 29:
YY_RULE_SETUP
#line 91 "verilog.l"
{ return K_LAND; }
	YY_BREAK
case 30:
YY_RULE_SETUP
#line 92 "verilog.l"
{ return K_NOR; }
	YY_BREAK
case 31:
YY_RULE_SETUP
#line 93 "verilog.l"
{ return K_NXOR; }
	YY_BREAK
case 32:
YY_RULE_SETUP
#line 94 "verilog.l"
{ return K_NXOR; }
	YY_BREAK
case 33:
YY_RULE_SETUP
#line 95 "verilog.l"
{ return K_NAND; }
	YY_BREAK
case 34:
YY_RULE_SETUP
#line 96 "verilog.l"
{ return K_TRIGGER; }
	YY_BREAK
case 35:
YY_RULE_SETUP
#line 97 "verilog.l"
{ return K_AAA; }
	YY_BREAK
case 36:
YY_RULE_SETUP
#line 98 "verilog.l"
{ return K_SIGNED; }
	YY_BREAK
case 37:
YY_RULE_SETUP
#line 99 "verilog.l"
{ return K_UNSIGNED; }
	YY_BREAK
case 38:
YY_RULE_SETUP
#line 100 "verilog.l"
{ return K_PLUSRANGE; }
	YY_BREAK
case 39:
YY_RULE_SETUP
#line 101 "verilog.l"
{ return K_MINUSRANGE; }
	YY_BREAK
/* ************************************************************************ */
/* Annex B */
/* B.1 All keywords */
case 40:
YY_RULE_SETUP
#line 107 "verilog.l"
{ return K_always; }
	YY_BREAK
case 41:
YY_RULE_SETUP
#line 108 "verilog.l"
{ return K_and; }
	YY_BREAK
case 42:
YY_RULE_SETUP
#line 109 "verilog.l"
{ return K_assign; }
	YY_BREAK
case 43:
YY_RULE_SETUP
#line 110 "verilog.l"
{ return K_automatic; }
	YY_BREAK
case 44:
YY_RULE_SETUP
#line 111 "verilog.l"
{ return K_begin; }
	YY_BREAK
case 45:
YY_RULE_SETUP
#line 112 "verilog.l"
{ return K_buf; }
	YY_BREAK
case 46:
YY_RULE_SETUP
#line 113 "verilog.l"
{ return K_bufif0; }
	YY_BREAK
case 47:
YY_RULE_SETUP
#line 114 "verilog.l"
{ return K_bufif1; }
	YY_BREAK
case 48:
YY_RULE_SETUP
#line 115 "verilog.l"
{ return K_case; }
	YY_BREAK
case 49:
YY_RULE_SETUP
#line 116 "verilog.l"
{ return K_casex; }
	YY_BREAK
case 50:
YY_RULE_SETUP
#line 117 "verilog.l"
{ return K_casez; }
	YY_BREAK
case 51:
YY_RULE_SETUP
#line 118 "verilog.l"
{ return K_cmos; }
	YY_BREAK
case 52:
YY_RULE_SETUP
#line 119 "verilog.l"
{ return K_deassign; }
	YY_BREAK
case 53:
YY_RULE_SETUP
#line 120 "verilog.l"
{ return K_default; }
	YY_BREAK
case 54:
YY_RULE_SETUP
#line 121 "verilog.l"
{ return K_defparam; }
	YY_BREAK
case 55:
YY_RULE_SETUP
#line 122 "verilog.l"
{ return K_disable; }
	YY_BREAK
case 56:
YY_RULE_SETUP
#line 123 "verilog.l"
{ return K_edge; }
	YY_BREAK
case 57:
YY_RULE_SETUP
#line 124 "verilog.l"
{ return K_else; }
	YY_BREAK
case 58:
YY_RULE_SETUP
#line 125 "verilog.l"
{ return K_end; }
	YY_BREAK
case 59:
YY_RULE_SETUP
#line 126 "verilog.l"
{ return K_endcase; }
	YY_BREAK
case 60:
YY_RULE_SETUP
#line 127 "verilog.l"
{ return K_endconfig; }
	YY_BREAK
case 61:
YY_RULE_SETUP
#line 128 "verilog.l"
{ return K_endfunction; }
	YY_BREAK
case 62:
YY_RULE_SETUP
#line 129 "verilog.l"
{ return K_endgenerate; }
	YY_BREAK
case 63:
YY_RULE_SETUP
#line 130 "verilog.l"
{ return K_endmodule; }
	YY_BREAK
case 64:
YY_RULE_SETUP
#line 131 "verilog.l"
{ return K_endprimitive; }
	YY_BREAK
case 65:
YY_RULE_SETUP
#line 132 "verilog.l"
{ return K_endspecify; }
	YY_BREAK
case 66:
YY_RULE_SETUP
#line 133 "verilog.l"
{ return K_endtable; }
	YY_BREAK
case 67:
YY_RULE_SETUP
#line 134 "verilog.l"
{ return K_endtask; }
	YY_BREAK
case 68:
YY_RULE_SETUP
#line 135 "verilog.l"
{ return K_event; }
	YY_BREAK
case 69:
YY_RULE_SETUP
#line 136 "verilog.l"
{ return K_for; }
	YY_BREAK
case 70:
YY_RULE_SETUP
#line 137 "verilog.l"
{ return K_force; }
	YY_BREAK
case 71:
YY_RULE_SETUP
#line 138 "verilog.l"
{ return K_forever; }
	YY_BREAK
case 72:
YY_RULE_SETUP
#line 139 "verilog.l"
{ return K_fork; }
	YY_BREAK
case 73:
YY_RULE_SETUP
#line 140 "verilog.l"
{ return K_function; }
	YY_BREAK
case 74:
YY_RULE_SETUP
#line 141 "verilog.l"
{ return K_generate; }
	YY_BREAK
case 75:
YY_RULE_SETUP
#line 142 "verilog.l"
{ return K_genvar; }
	YY_BREAK
case 76:
YY_RULE_SETUP
#line 143 "verilog.l"
{ return K_highz0; }
	YY_BREAK
case 77:
YY_RULE_SETUP
#line 144 "verilog.l"
{ return K_highz1; }
	YY_BREAK
case 78:
YY_RULE_SETUP
#line 145 "verilog.l"
{ return K_if; }
	YY_BREAK
case 79:
YY_RULE_SETUP
#line 146 "verilog.l"
{ return K_ifnone; }
	YY_BREAK
case 80:
YY_RULE_SETUP
#line 147 "verilog.l"
{ return K_initial; }
	YY_BREAK
case 81:
YY_RULE_SETUP
#line 148 "verilog.l"
{ return K_inout; }
	YY_BREAK
case 82:
YY_RULE_SETUP
#line 149 "verilog.l"
{ return K_input; }
	YY_BREAK
case 83:
YY_RULE_SETUP
#line 150 "verilog.l"
{ return K_integer; }
	YY_BREAK
case 84:
YY_RULE_SETUP
#line 151 "verilog.l"
{ return K_join; }
	YY_BREAK
case 85:
YY_RULE_SETUP
#line 152 "verilog.l"
{ return K_large; }
	YY_BREAK
case 86:
YY_RULE_SETUP
#line 153 "verilog.l"
{ return K_library; }
	YY_BREAK
case 87:
YY_RULE_SETUP
#line 154 "verilog.l"
{ return K_localparam; }
	YY_BREAK
case 88:
YY_RULE_SETUP
#line 155 "verilog.l"
{ return K_macromodule; }
	YY_BREAK
case 89:
YY_RULE_SETUP
#line 156 "verilog.l"
{ return K_medium; }
	YY_BREAK
case 90:
YY_RULE_SETUP
#line 157 "verilog.l"
{ return K_module; }
	YY_BREAK
case 91:
YY_RULE_SETUP
#line 158 "verilog.l"
{ return K_nand; }
	YY_BREAK
case 92:
YY_RULE_SETUP
#line 159 "verilog.l"
{ return K_negedge; }
	YY_BREAK
case 93:
YY_RULE_SETUP
#line 160 "verilog.l"
{ return K_nmos; }
	YY_BREAK
case 94:
YY_RULE_SETUP
#line 161 "verilog.l"
{ return K_nor; }
	YY_BREAK
case 95:
YY_RULE_SETUP
#line 162 "verilog.l"
{ return K_not; }
	YY_BREAK
case 96:
YY_RULE_SETUP
#line 163 "verilog.l"
{ return K_notif0; }
	YY_BREAK
case 97:
YY_RULE_SETUP
#line 164 "verilog.l"
{ return K_notif1; }
	YY_BREAK
case 98:
YY_RULE_SETUP
#line 165 "verilog.l"
{ return K_or; }
	YY_BREAK
case 99:
YY_RULE_SETUP
#line 166 "verilog.l"
{ return K_output; }
	YY_BREAK
case 100:
YY_RULE_SETUP
#line 167 "verilog.l"
{ return K_parameter; }
	YY_BREAK
case 101:
YY_RULE_SETUP
#line 168 "verilog.l"
{ return K_pmos; }
	YY_BREAK
case 102:
YY_RULE_SETUP
#line 169 "verilog.l"
{ return K_posedge; }
	YY_BREAK
case 103:
YY_RULE_SETUP
#line 170 "verilog.l"
{ return K_primitive; }
	YY_BREAK
case 104:
YY_RULE_SETUP
#line 171 "verilog.l"
{ return K_pull0; }
	YY_BREAK
case 105:
YY_RULE_SETUP
#line 172 "verilog.l"
{ return K_pull1; }
	YY_BREAK
case 106:
YY_RULE_SETUP
#line 173 "verilog.l"
{ return K_pulldown; }
	YY_BREAK
case 107:
YY_RULE_SETUP
#line 174 "verilog.l"
{ return K_pullup; }
	YY_BREAK
case 108:
YY_RULE_SETUP
#line 175 "verilog.l"
{ return K_rcmos; }
	YY_BREAK
case 109:
YY_RULE_SETUP
#line 176 "verilog.l"
{ return K_real; }
	YY_BREAK
case 110:
YY_RULE_SETUP
#line 177 "verilog.l"
{ return K_realtime; }
	YY_BREAK
case 111:
YY_RULE_SETUP
#line 178 "verilog.l"
{ return K_reg; }
	YY_BREAK
case 112:
YY_RULE_SETUP
#line 179 "verilog.l"
{ return K_release; }
	YY_BREAK
case 113:
YY_RULE_SETUP
#line 180 "verilog.l"
{ return K_repeat; }
	YY_BREAK
case 114:
YY_RULE_SETUP
#line 181 "verilog.l"
{ return K_rnmos; }
	YY_BREAK
case 115:
YY_RULE_SETUP
#line 182 "verilog.l"
{ return K_rpmos; }
	YY_BREAK
case 116:
YY_RULE_SETUP
#line 183 "verilog.l"
{ return K_rtran; }
	YY_BREAK
case 117:
YY_RULE_SETUP
#line 184 "verilog.l"
{ return K_rtranif0; }
	YY_BREAK
case 118:
YY_RULE_SETUP
#line 185 "verilog.l"
{ return K_rtranif1; }
	YY_BREAK
case 119:
YY_RULE_SETUP
#line 186 "verilog.l"
{ return K_scalared; }
	YY_BREAK
case 120:
YY_RULE_SETUP
#line 187 "verilog.l"
{ return K_signed; }
	YY_BREAK
case 121:
YY_RULE_SETUP
#line 188 "verilog.l"
{ return K_small; }
	YY_BREAK
case 122:
YY_RULE_SETUP
#line 189 "verilog.l"
{ return K_specify; }
	YY_BREAK
case 123:
YY_RULE_SETUP
#line 190 "verilog.l"
{ return K_specparam; }
	YY_BREAK
case 124:
YY_RULE_SETUP
#line 191 "verilog.l"
{ return K_strong0; }
	YY_BREAK
case 125:
YY_RULE_SETUP
#line 192 "verilog.l"
{ return K_strong1; }
	YY_BREAK
case 126:
YY_RULE_SETUP
#line 193 "verilog.l"
{ return K_supply0; }
	YY_BREAK
case 127:
YY_RULE_SETUP
#line 194 "verilog.l"
{ return K_supply1; }
	YY_BREAK
case 128:
YY_RULE_SETUP
#line 195 "verilog.l"
{ return K_table; }
	YY_BREAK
case 129:
YY_RULE_SETUP
#line 196 "verilog.l"
{ return K_task; }
	YY_BREAK
case 130:
YY_RULE_SETUP
#line 197 "verilog.l"
{ return K_time; }
	YY_BREAK
case 131:
YY_RULE_SETUP
#line 198 "verilog.l"
{ return K_tran; }
	YY_BREAK
case 132:
YY_RULE_SETUP
#line 199 "verilog.l"
{ return K_tranif0; }
	YY_BREAK
case 133:
YY_RULE_SETUP
#line 200 "verilog.l"
{ return K_tranif1; }
	YY_BREAK
case 134:
YY_RULE_SETUP
#line 201 "verilog.l"
{ return K_tri; }
	YY_BREAK
case 135:
YY_RULE_SETUP
#line 202 "verilog.l"
{ return K_tri0; }
	YY_BREAK
case 136:
YY_RULE_SETUP
#line 203 "verilog.l"
{ return K_tri1; }
	YY_BREAK
case 137:
YY_RULE_SETUP
#line 204 "verilog.l"
{ return K_triand; }
	YY_BREAK
case 138:
YY_RULE_SETUP
#line 205 "verilog.l"
{ return K_trior; }
	YY_BREAK
case 139:
YY_RULE_SETUP
#line 206 "verilog.l"
{ return K_trireg; }
	YY_BREAK
case 140:
YY_RULE_SETUP
#line 207 "verilog.l"
{ return K_unsigned; }
	YY_BREAK
case 141:
YY_RULE_SETUP
#line 208 "verilog.l"
{ return K_vectored; }
	YY_BREAK
case 142:
YY_RULE_SETUP
#line 209 "verilog.l"
{ return K_wait; }
	YY_BREAK
case 143:
YY_RULE_SETUP
#line 210 "verilog.l"
{ return K_wand; }
	YY_BREAK
case 144:
YY_RULE_SETUP
#line 211 "verilog.l"
{ return K_weak0; }
	YY_BREAK
case 145:
YY_RULE_SETUP
#line 212 "verilog.l"
{ return K_weak1; }
	YY_BREAK
case 146:
YY_RULE_SETUP
#line 213 "verilog.l"
{ return K_while; }
	YY_BREAK
case 147:
YY_RULE_SETUP
#line 214 "verilog.l"
{ return K_wire; }
	YY_BREAK
case 148:
YY_RULE_SETUP
#line 215 "verilog.l"
{ return K_wor; }
	YY_BREAK
case 149:
YY_RULE_SETUP
#line 216 "verilog.l"
{ return K_xnor; }
	YY_BREAK
case 150:
YY_RULE_SETUP
#line 217 "verilog.l"
{ return K_xor; }
	YY_BREAK
/* B.2 Configuration */
case 151:
YY_RULE_SETUP
#line 220 "verilog.l"
{ return K_design; }
	YY_BREAK
case 152:
YY_RULE_SETUP
#line 221 "verilog.l"
{ return K_instance; }
	YY_BREAK
case 153:
YY_RULE_SETUP
#line 222 "verilog.l"
{ return K_cell; }
	YY_BREAK
case 154:
YY_RULE_SETUP
#line 223 "verilog.l"
{ return K_use; }
	YY_BREAK
case 155:
YY_RULE_SETUP
#line 224 "verilog.l"
{ return K_liblist; }
	YY_BREAK
/* B.3 Library */
case 156:
YY_RULE_SETUP
#line 227 "verilog.l"
{ return K_include; }
	YY_BREAK
case 157:
YY_RULE_SETUP
#line 228 "verilog.l"
{ return K_incdir; }
	YY_BREAK
/* ************************************************************************ */
/* Annex C */
/* System tasks and functions */
case 158:
YY_RULE_SETUP
#line 233 "verilog.l"
{ return K_countdrivers; }
	YY_BREAK
case 159:
YY_RULE_SETUP
#line 234 "verilog.l"
{ return K_getpattern; }
	YY_BREAK
case 160:
YY_RULE_SETUP
#line 235 "verilog.l"
{ return K_incsave; }
	YY_BREAK
case 161:
YY_RULE_SETUP
#line 236 "verilog.l"
{ return K_input; }
	YY_BREAK
case 162:
YY_RULE_SETUP
#line 237 "verilog.l"
{ return K_key; }
	YY_BREAK
case 163:
YY_RULE_SETUP
#line 238 "verilog.l"
{ return K_list; }
	YY_BREAK
case 164:
YY_RULE_SETUP
#line 239 "verilog.l"
{ return K_log; }
	YY_BREAK
case 165:
YY_RULE_SETUP
#line 240 "verilog.l"
{ return K_nokey; }
	YY_BREAK
case 166:
YY_RULE_SETUP
#line 241 "verilog.l"
{ return K_nolog; }
	YY_BREAK
case 167:
YY_RULE_SETUP
#line 242 "verilog.l"
{ return K_reset; }
	YY_BREAK
case 168:
YY_RULE_SETUP
#line 243 "verilog.l"
{ return K_reset_count; }
	YY_BREAK
case 169:
YY_RULE_SETUP
#line 244 "verilog.l"
{ return K_reset_value; }
	YY_BREAK
case 170:
YY_RULE_SETUP
#line 245 "verilog.l"
{ return K_restart; }
	YY_BREAK
case 171:
YY_RULE_SETUP
#line 246 "verilog.l"
{ return K_save; }
	YY_BREAK
case 172:
YY_RULE_SETUP
#line 247 "verilog.l"
{ return K_scale; }
	YY_BREAK
case 173:
YY_RULE_SETUP
#line 248 "verilog.l"
{ return K_scope; }
	YY_BREAK
case 174:
YY_RULE_SETUP
#line 249 "verilog.l"
{ return K_showscopes; }
	YY_BREAK
case 175:
YY_RULE_SETUP
#line 250 "verilog.l"
{ return K_showvars; }
	YY_BREAK
case 176:
YY_RULE_SETUP
#line 251 "verilog.l"
{ return K_sreadmemb; }
	YY_BREAK
case 177:
YY_RULE_SETUP
#line 252 "verilog.l"
{ return K_sreadmemh; }
	YY_BREAK
/* ************************************************************************ */
/* Annex D */
/* Compiler directives */
case 178:
YY_RULE_SETUP
#line 257 "verilog.l"
{ comment_enter = YY_START; BEGIN(LCOMMENT); }
	YY_BREAK
case 179:
YY_RULE_SETUP
#line 258 "verilog.l"
{ comment_enter = YY_START; BEGIN(LCOMMENT); }
	YY_BREAK
case 180:
YY_RULE_SETUP
#line 259 "verilog.l"
{ comment_enter = YY_START; BEGIN(LCOMMENT); }
	YY_BREAK
case 181:
YY_RULE_SETUP
#line 260 "verilog.l"
{ comment_enter = YY_START; BEGIN(LCOMMENT); }
	YY_BREAK
case 182:
YY_RULE_SETUP
#line 261 "verilog.l"
{ comment_enter = YY_START; BEGIN(LCOMMENT); }
	YY_BREAK
case 183:
YY_RULE_SETUP
#line 262 "verilog.l"
{ comment_enter = YY_START; BEGIN(LCOMMENT); }
	YY_BREAK
case 184:
YY_RULE_SETUP
#line 271 "verilog.l"
{ return verilog_text[0]; }
	YY_BREAK
case 185:
YY_RULE_SETUP
#line 273 "verilog.l"
{ BEGIN(CSTRING); }
	YY_BREAK
case 186:
YY_RULE_SETUP
#line 274 "verilog.l"
{ yymore(); }
	YY_BREAK
case 187:
/* rule 187 can match eol */
YY_RULE_SETUP
#line 275 "verilog.l"
{ BEGIN(0);
                yylval.text = strdup(verilog_text);
		cerr << file << " : " << line << ": Missing close quote of string.\n";
		line +=1;
		return STRING; }
	YY_BREAK
case 188:
YY_RULE_SETUP
#line 280 "verilog.l"
{ BEGIN(0);
                yylval.text = strdup(verilog_text);
		yylval.text[strlen(verilog_text)-1] = 0;
		return STRING; }
	YY_BREAK
case 189:
YY_RULE_SETUP
#line 284 "verilog.l"
{ yymore(); }
	YY_BREAK
case 190:
YY_RULE_SETUP
#line 287 "verilog.l"
{ return '_'; }
	YY_BREAK
case 191:
YY_RULE_SETUP
#line 288 "verilog.l"
{ return '+'; }
	YY_BREAK
case 192:
YY_RULE_SETUP
#line 289 "verilog.l"
{ return '%'; }
	YY_BREAK
case 193:
YY_RULE_SETUP
#line 290 "verilog.l"
{ return '*'; }
	YY_BREAK
case 194:
YY_RULE_SETUP
#line 291 "verilog.l"
{ return 'r'; }
	YY_BREAK
case 195:
YY_RULE_SETUP
#line 292 "verilog.l"
{ return 'P'; }
	YY_BREAK
case 196:
YY_RULE_SETUP
#line 293 "verilog.l"
{ return 'f'; }
	YY_BREAK
case 197:
YY_RULE_SETUP
#line 294 "verilog.l"
{ return 'N'; }
	YY_BREAK
case 198:
YY_RULE_SETUP
#line 295 "verilog.l"
{ return 'F'; }
	YY_BREAK
case 199:
YY_RULE_SETUP
#line 296 "verilog.l"
{ return 'R'; }
	YY_BREAK
case 200:
YY_RULE_SETUP
#line 297 "verilog.l"
{ return 'b'; }
	YY_BREAK
case 201:
YY_RULE_SETUP
#line 298 "verilog.l"
{ return 'f'; }
	YY_BREAK
case 202:
YY_RULE_SETUP
#line 299 "verilog.l"
{ return 'r'; }
	YY_BREAK
case 203:
YY_RULE_SETUP
#line 300 "verilog.l"
{ return 'x'; }
	YY_BREAK
case 204:
YY_RULE_SETUP
#line 301 "verilog.l"
{ return verilog_text[0]; }
	YY_BREAK
case 205:
YY_RULE_SETUP
#line 303 "verilog.l"
{
  yylval.text =strdup(verilog_text);
  return DIDENTIFIER; }
	YY_BREAK
case 206:
YY_RULE_SETUP
#line 307 "verilog.l"
{
  yylval.text =strdup(verilog_text);
  return IDENTIFIER; }
	YY_BREAK
case 207:
YY_RULE_SETUP
#line 311 "verilog.l"
{
  yylval.text = strdup(verilog_text);
  return HIDENTIFIER; }
	YY_BREAK
case 208:
/* rule 208 can match eol */
YY_RULE_SETUP
#line 315 "verilog.l"
{
  yylval.text = strdup(verilog_text);
  return IDENTIFIER; }
	YY_BREAK
case 209:
YY_RULE_SETUP
#line 319 "verilog.l"
{
  yylval.text = strdup(verilog_text);
  return SYSTEM_IDENTIFIER; }
	YY_BREAK
case 210:
YY_RULE_SETUP
#line 323 "verilog.l"
{
  char*cp = verilog_text+1;
  while (! (isalpha(*cp) || (*cp == '_')))
    cp += 1;
  yylval.text = strdup(cp);
  return PORTNAME; }
	YY_BREAK
case 211:
YY_RULE_SETUP
#line 330 "verilog.l"
{
  yylval.text = strdup(verilog_text);
  return NUMBER; }
	YY_BREAK
case 212:
YY_RULE_SETUP
#line 333 "verilog.l"
{
  yylval.text = strdup(verilog_text);
  return NUMBER; }
	YY_BREAK
case 213:
YY_RULE_SETUP
#line 336 "verilog.l"
{
  yylval.text = strdup(verilog_text);
  return NUMBER; }
	YY_BREAK
case 214:
YY_RULE_SETUP
#line 339 "verilog.l"
{
  yylval.text = strdup(verilog_text);
  return NUMBER; }
	YY_BREAK
case 215:
YY_RULE_SETUP
#line 343 "verilog.l"
{
  yylval.text = strdup(verilog_text);
  return NUMBER; }
	YY_BREAK
case 216:
YY_RULE_SETUP
#line 346 "verilog.l"
{
  yylval.text = strdup(verilog_text);
  return NUMBER; }
	YY_BREAK
case 217:
YY_RULE_SETUP
#line 349 "verilog.l"
{
  yylval.text = strdup(verilog_text);
  return NUMBER; }
	YY_BREAK
case 218:
YY_RULE_SETUP
#line 352 "verilog.l"
{
  yylval.text = strdup(verilog_text);
  return NUMBER; }
	YY_BREAK
case 219:
YY_RULE_SETUP
#line 356 "verilog.l"
{
  yylval.text = strdup(verilog_text);
  return NUMBER; }
	YY_BREAK
case 220:
YY_RULE_SETUP
#line 360 "verilog.l"
{
  yylval.text = strdup(verilog_text);
  return REALTIME; }
	YY_BREAK
case 221:
YY_RULE_SETUP
#line 364 "verilog.l"
{
  yylval.text = strdup(verilog_text);
  return REALTIME; }
	YY_BREAK
case 222:
YY_RULE_SETUP
#line 368 "verilog.l"
{ return K_ATTRIBUTE; }
	YY_BREAK
case 223:
YY_RULE_SETUP
#line 370 "verilog.l"
{ comment_enter = YY_START; BEGIN(LCOMMENT); }
	YY_BREAK
case 224:
YY_RULE_SETUP
#line 371 "verilog.l"
{ comment_enter = YY_START; BEGIN(LCOMMENT); }
	YY_BREAK
case 225:
YY_RULE_SETUP
#line 372 "verilog.l"
{ comment_enter = YY_START; BEGIN(LCOMMENT); }
	YY_BREAK
case 226:
YY_RULE_SETUP
#line 373 "verilog.l"
{ comment_enter = YY_START; BEGIN(LCOMMENT); }
	YY_BREAK
case 227:
YY_RULE_SETUP
#line 374 "verilog.l"
{ comment_enter = YY_START; BEGIN(LCOMMENT); }
	YY_BREAK
case 228:
YY_RULE_SETUP
#line 375 "verilog.l"
{ comment_enter = YY_START; BEGIN(LCOMMENT); }
	YY_BREAK
case 229:
YY_RULE_SETUP
#line 376 "verilog.l"
{ comment_enter = YY_START; BEGIN(LCOMMENT); }
	YY_BREAK
case 230:
YY_RULE_SETUP
#line 377 "verilog.l"
{ comment_enter = YY_START; BEGIN(LCOMMENT); }
	YY_BREAK
case 231:
YY_RULE_SETUP
#line 378 "verilog.l"
{ comment_enter = YY_START; BEGIN(LCOMMENT); }
	YY_BREAK
case 232:
YY_RULE_SETUP
#line 379 "verilog.l"
{ comment_enter = YY_START; BEGIN(LCOMMENT); }
	YY_BREAK
case 233:
YY_RULE_SETUP
#line 380 "verilog.l"
{ comment_enter = YY_START; BEGIN(LCOMMENT); }
	YY_BREAK
case 234:
YY_RULE_SETUP
#line 381 "verilog.l"
{ comment_enter = YY_START; BEGIN(LCOMMENT); }
	YY_BREAK
case 235:
YY_RULE_SETUP
#line 382 "verilog.l"
{ comment_enter = YY_START; BEGIN(LCOMMENT); }
	YY_BREAK
case 236:
YY_RULE_SETUP
#line 383 "verilog.l"
{ comment_enter = YY_START; BEGIN(LCOMMENT); }
	YY_BREAK
case 237:
YY_RULE_SETUP
#line 384 "verilog.l"
{ comment_enter = YY_START; BEGIN(LCOMMENT); }
	YY_BREAK
case 238:
YY_RULE_SETUP
#line 385 "verilog.l"
{ comment_enter = YY_START; BEGIN(LCOMMENT); }
	YY_BREAK
case 239:
YY_RULE_SETUP
#line 386 "verilog.l"
{ comment_enter = YY_START; BEGIN(LCOMMENT); }
	YY_BREAK
case 240:
YY_RULE_SETUP
#line 387 "verilog.l"
{ comment_enter = YY_START; BEGIN(LCOMMENT); }
	YY_BREAK
case 241:
YY_RULE_SETUP
#line 388 "verilog.l"
{ comment_enter = YY_START; BEGIN(LCOMMENT); }
	YY_BREAK
case 242:
YY_RULE_SETUP
#line 389 "verilog.l"
{ comment_enter = YY_START; BEGIN(LCOMMENT); }
	YY_BREAK
case 243:
YY_RULE_SETUP
#line 390 "verilog.l"
{ comment_enter = YY_START; BEGIN(LCOMMENT); }
	YY_BREAK
case 244:
YY_RULE_SETUP
#line 391 "verilog.l"
{ comment_enter = YY_START; BEGIN(LCOMMENT); }
	YY_BREAK
case 245:
YY_RULE_SETUP
#line 392 "verilog.l"
{ comment_enter = YY_START; BEGIN(LCOMMENT); }
	YY_BREAK
case 246:
YY_RULE_SETUP
#line 393 "verilog.l"
{ comment_enter = YY_START; BEGIN(LCOMMENT); }
	YY_BREAK
case 247:
YY_RULE_SETUP
#line 394 "verilog.l"
{ comment_enter = YY_START; BEGIN(LCOMMENT); }
	YY_BREAK
case 248:
YY_RULE_SETUP
#line 395 "verilog.l"
{ comment_enter = YY_START; BEGIN(LCOMMENT); }
	YY_BREAK
case 249:
YY_RULE_SETUP
#line 398 "verilog.l"
{
  cerr << file << " : " << line << ": unmatched character (";
  if (isgraph(verilog_text[0]))
    cerr << verilog_text[0];
  else
    cerr << (unsigned)verilog_text[0];
  cerr << ")" << endl; }
	YY_BREAK
case 250:
YY_RULE_SETUP
#line 406 "verilog.l"
ECHO;
	YY_BREAK
#line 3882 "_lexor.cc"
case YY_STATE_EOF(INITIAL):
case YY_STATE_EOF(ATTRIBUTE):
case YY_STATE_EOF(CCOMMENT):
case YY_STATE_EOF(LCOMMENT):
case YY_STATE_EOF(CSTRING):
case YY_STATE_EOF(UDPTABLE):
case YY_STATE_EOF(PPTIMESCALE):
	yyterminate();

	case YY_END_OF_BUFFER:
		{
		/* Amount of text matched not including the EOB char. */
		int yy_amount_of_matched_text = (int) (yy_cp - (yytext_ptr)) - 1;

		/* Undo the effects of YY_DO_BEFORE_ACTION. */
		*yy_cp = (yy_hold_char);
		YY_RESTORE_YY_MORE_OFFSET

		if ( YY_CURRENT_BUFFER_LVALUE->yy_buffer_status == YY_BUFFER_NEW )
			{
			/* We're scanning a new file or input source.  It's
			 * possible that this happened because the user
			 * just pointed verilog_in at a new source and called
			 * verilog_lex().  If so, then we have to assure
			 * consistency between YY_CURRENT_BUFFER and our
			 * globals.  Here is the right place to do so, because
			 * this is the first action (other than possibly a
			 * back-up) that will match for the new input source.
			 */
			(yy_n_chars) = YY_CURRENT_BUFFER_LVALUE->yy_n_chars;
			YY_CURRENT_BUFFER_LVALUE->yy_input_file = verilog_in;
			YY_CURRENT_BUFFER_LVALUE->yy_buffer_status = YY_BUFFER_NORMAL;
			}

		/* Note that here we test for yy_c_buf_p "<=" to the position
		 * of the first EOB in the buffer, since yy_c_buf_p will
		 * already have been incremented past the NUL character
		 * (since all states make transitions on EOB to the
		 * end-of-buffer state).  Contrast this with the test
		 * in input().
		 */
		if ( (yy_c_buf_p) <= &YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[(yy_n_chars)] )
			{ /* This was really a NUL. */
			yy_state_type yy_next_state;

			(yy_c_buf_p) = (yytext_ptr) + yy_amount_of_matched_text;

			yy_current_state = yy_get_previous_state(  );

			/* Okay, we're now positioned to make the NUL
			 * transition.  We couldn't have
			 * yy_get_previous_state() go ahead and do it
			 * for us because it doesn't know how to deal
			 * with the possibility of jamming (and we don't
			 * want to build jamming into it because then it
			 * will run more slowly).
			 */

			yy_next_state = yy_try_NUL_trans( yy_current_state );

			yy_bp = (yytext_ptr) + YY_MORE_ADJ;

			if ( yy_next_state )
				{
				/* Consume the NUL. */
				yy_cp = ++(yy_c_buf_p);
				yy_current_state = yy_next_state;
				goto yy_match;
				}

			else
				{
				yy_cp = (yy_c_buf_p);
				goto yy_find_action;
				}
			}

		else switch ( yy_get_next_buffer(  ) )
			{
			case EOB_ACT_END_OF_FILE:
				{
				(yy_did_buffer_switch_on_eof) = 0;

				if ( verilog_wrap( ) )
					{
					/* Note: because we've taken care in
					 * yy_get_next_buffer() to have set up
					 * verilog_text, we can now set up
					 * yy_c_buf_p so that if some total
					 * hoser (like flex itself) wants to
					 * call the scanner after we return the
					 * YY_NULL, it'll still work - another
					 * YY_NULL will get returned.
					 */
					(yy_c_buf_p) = (yytext_ptr) + YY_MORE_ADJ;

					yy_act = YY_STATE_EOF(YY_START);
					goto do_action;
					}

				else
					{
					if ( ! (yy_did_buffer_switch_on_eof) )
						YY_NEW_FILE;
					}
				break;
				}

			case EOB_ACT_CONTINUE_SCAN:
				(yy_c_buf_p) =
					(yytext_ptr) + yy_amount_of_matched_text;

				yy_current_state = yy_get_previous_state(  );

				yy_cp = (yy_c_buf_p);
				yy_bp = (yytext_ptr) + YY_MORE_ADJ;
				goto yy_match;

			case EOB_ACT_LAST_MATCH:
				(yy_c_buf_p) =
				&YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[(yy_n_chars)];

				yy_current_state = yy_get_previous_state(  );

				yy_cp = (yy_c_buf_p);
				yy_bp = (yytext_ptr) + YY_MORE_ADJ;
				goto yy_find_action;
			}
		break;
		}

	default:
		YY_FATAL_ERROR(
			"fatal flex scanner internal error--no action found" );
	} /* end of action switch */
		} /* end of scanning one token */
	} /* end of user's declarations */
} /* end of verilog_lex */

/* yy_get_next_buffer - try to read in a new buffer
 *
 * Returns a code representing an action:
 *	EOB_ACT_LAST_MATCH -
 *	EOB_ACT_CONTINUE_SCAN - continue scanning from current position
 *	EOB_ACT_END_OF_FILE - end of file
 */
static int yy_get_next_buffer (void)
{
    	char *dest = YY_CURRENT_BUFFER_LVALUE->yy_ch_buf;
	char *source = (yytext_ptr);
	yy_size_t number_to_move, i;
	int ret_val;

	if ( (yy_c_buf_p) > &YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[(yy_n_chars) + 1] )
		YY_FATAL_ERROR(
		"fatal flex scanner internal error--end of buffer missed" );

	if ( YY_CURRENT_BUFFER_LVALUE->yy_fill_buffer == 0 )
		{ /* Don't try to fill the buffer, so this is an EOF. */
		if ( (yy_c_buf_p) - (yytext_ptr) - YY_MORE_ADJ == 1 )
			{
			/* We matched a single character, the EOB, so
			 * treat this as a final EOF.
			 */
			return EOB_ACT_END_OF_FILE;
			}

		else
			{
			/* We matched some text prior to the EOB, first
			 * process it.
			 */
			return EOB_ACT_LAST_MATCH;
			}
		}

	/* Try to read more data. */

	/* First move last chars to start of buffer. */
	number_to_move = (yy_size_t) ((yy_c_buf_p) - (yytext_ptr)) - 1;

	for ( i = 0; i < number_to_move; ++i )
		*(dest++) = *(source++);

	if ( YY_CURRENT_BUFFER_LVALUE->yy_buffer_status == YY_BUFFER_EOF_PENDING )
		/* don't do the read, it's not guaranteed to return an EOF,
		 * just force an EOF
		 */
		YY_CURRENT_BUFFER_LVALUE->yy_n_chars = (yy_n_chars) = 0;

	else
		{
			yy_size_t num_to_read =
			YY_CURRENT_BUFFER_LVALUE->yy_buf_size - number_to_move - 1;

		while ( num_to_read <= 0 )
			{ /* Not enough room in the buffer - grow it. */

			/* just a shorter name for the current buffer */
			YY_BUFFER_STATE b = YY_CURRENT_BUFFER_LVALUE;

			int yy_c_buf_p_offset =
				(int) ((yy_c_buf_p) - b->yy_ch_buf);

			if ( b->yy_is_our_buffer )
				{
				yy_size_t new_size = b->yy_buf_size * 2;

				if ( new_size <= 0 )
					b->yy_buf_size += b->yy_buf_size / 8;
				else
					b->yy_buf_size *= 2;

				b->yy_ch_buf = (char *)
					/* Include room in for 2 EOB chars. */
					verilog_realloc((void *) b->yy_ch_buf,b->yy_buf_size + 2  );
				}
			else
				/* Can't grow it, we don't own it. */
				b->yy_ch_buf = 0;

			if ( ! b->yy_ch_buf )
				YY_FATAL_ERROR(
				"fatal error - scanner input buffer overflow" );

			(yy_c_buf_p) = &b->yy_ch_buf[yy_c_buf_p_offset];

			num_to_read = YY_CURRENT_BUFFER_LVALUE->yy_buf_size -
						number_to_move - 1;

			}

		if ( num_to_read > YY_READ_BUF_SIZE )
			num_to_read = YY_READ_BUF_SIZE;

		/* Read in more data. */
		YY_INPUT( (&YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[number_to_move]),
			(yy_n_chars), num_to_read );

		YY_CURRENT_BUFFER_LVALUE->yy_n_chars = (yy_n_chars);
		}

	if ( (yy_n_chars) == 0 )
		{
		if ( number_to_move == YY_MORE_ADJ )
			{
			ret_val = EOB_ACT_END_OF_FILE;
			verilog_restart(verilog_in  );
			}

		else
			{
			ret_val = EOB_ACT_LAST_MATCH;
			YY_CURRENT_BUFFER_LVALUE->yy_buffer_status =
				YY_BUFFER_EOF_PENDING;
			}
		}

	else
		ret_val = EOB_ACT_CONTINUE_SCAN;

	if ((int) ((yy_n_chars) + number_to_move) > YY_CURRENT_BUFFER_LVALUE->yy_buf_size) {
		/* Extend the array by 50%, plus the number we really need. */
		int new_size = (yy_n_chars) + number_to_move + ((yy_n_chars) >> 1);
		YY_CURRENT_BUFFER_LVALUE->yy_ch_buf = (char *) verilog_realloc((void *) YY_CURRENT_BUFFER_LVALUE->yy_ch_buf,new_size  );
		if ( ! YY_CURRENT_BUFFER_LVALUE->yy_ch_buf )
			YY_FATAL_ERROR( "out of dynamic memory in yy_get_next_buffer()" );
	}

	(yy_n_chars) += number_to_move;
	YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[(yy_n_chars)] = YY_END_OF_BUFFER_CHAR;
	YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[(yy_n_chars) + 1] = YY_END_OF_BUFFER_CHAR;

	(yytext_ptr) = &YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[0];

	return ret_val;
}

/* yy_get_previous_state - get the state just before the EOB char was reached */

    static yy_state_type yy_get_previous_state (void)
{
	yy_state_type yy_current_state;
	char *yy_cp;
    
	yy_current_state = (yy_start);
	yy_current_state += YY_AT_BOL();

	for ( yy_cp = (yytext_ptr) + YY_MORE_ADJ; yy_cp < (yy_c_buf_p); ++yy_cp )
		{
		YY_CHAR yy_c = (*yy_cp ? yy_ec[YY_SC_TO_UI(*yy_cp)] : 1);
		if ( yy_accept[yy_current_state] )
			{
			(yy_last_accepting_state) = yy_current_state;
			(yy_last_accepting_cpos) = yy_cp;
			}
		while ( yy_chk[yy_base[yy_current_state] + yy_c] != yy_current_state )
			{
			yy_current_state = (int) yy_def[yy_current_state];
			if ( yy_current_state >= 1185 )
				yy_c = yy_meta[(unsigned int) yy_c];
			}
		yy_current_state = yy_nxt[yy_base[yy_current_state] + (unsigned int) yy_c];
		}

	return yy_current_state;
}

/* yy_try_NUL_trans - try to make a transition on the NUL character
 *
 * synopsis
 *	next_state = yy_try_NUL_trans( current_state );
 */
    static yy_state_type yy_try_NUL_trans  (yy_state_type yy_current_state )
{
	int yy_is_jam;
    	char *yy_cp = (yy_c_buf_p);

	YY_CHAR yy_c = 1;
	if ( yy_accept[yy_current_state] )
		{
		(yy_last_accepting_state) = yy_current_state;
		(yy_last_accepting_cpos) = yy_cp;
		}
	while ( yy_chk[yy_base[yy_current_state] + yy_c] != yy_current_state )
		{
		yy_current_state = (int) yy_def[yy_current_state];
		if ( yy_current_state >= 1185 )
			yy_c = yy_meta[(unsigned int) yy_c];
		}
	yy_current_state = yy_nxt[yy_base[yy_current_state] + (unsigned int) yy_c];
	yy_is_jam = (yy_current_state == 1184);

		return yy_is_jam ? 0 : yy_current_state;
}

#ifndef YY_NO_UNPUT

    static void yyunput (int c, char * yy_bp )
{
	char *yy_cp;
    
    yy_cp = (yy_c_buf_p);

	/* undo effects of setting up verilog_text */
	*yy_cp = (yy_hold_char);

	if ( yy_cp < YY_CURRENT_BUFFER_LVALUE->yy_ch_buf + 2 )
		{ /* need to shift things up to make room */
		/* +2 for EOB chars. */
		yy_size_t number_to_move = (yy_n_chars) + 2;
		char *dest = &YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[
					YY_CURRENT_BUFFER_LVALUE->yy_buf_size + 2];
		char *source =
				&YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[number_to_move];

		while ( source > YY_CURRENT_BUFFER_LVALUE->yy_ch_buf )
			*--dest = *--source;

		yy_cp += (int) (dest - source);
		yy_bp += (int) (dest - source);
		YY_CURRENT_BUFFER_LVALUE->yy_n_chars =
			(yy_n_chars) = YY_CURRENT_BUFFER_LVALUE->yy_buf_size;

		if ( yy_cp < YY_CURRENT_BUFFER_LVALUE->yy_ch_buf + 2 )
			YY_FATAL_ERROR( "flex scanner push-back overflow" );
		}

	*--yy_cp = (char) c;

	(yytext_ptr) = yy_bp;
	(yy_hold_char) = *yy_cp;
	(yy_c_buf_p) = yy_cp;
}

#endif

#ifndef YY_NO_INPUT
#ifdef __cplusplus
    static int yyinput (void)
#else
    static int input  (void)
#endif

{
	int c;
    
	*(yy_c_buf_p) = (yy_hold_char);

	if ( *(yy_c_buf_p) == YY_END_OF_BUFFER_CHAR )
		{
		/* yy_c_buf_p now points to the character we want to return.
		 * If this occurs *before* the EOB characters, then it's a
		 * valid NUL; if not, then we've hit the end of the buffer.
		 */
		if ( (yy_c_buf_p) < &YY_CURRENT_BUFFER_LVALUE->yy_ch_buf[(yy_n_chars)] )
			/* This was really a NUL. */
			*(yy_c_buf_p) = '\0';

		else
			{ /* need more input */
			yy_size_t offset = (yy_c_buf_p) - (yytext_ptr);
			++(yy_c_buf_p);

			switch ( yy_get_next_buffer(  ) )
				{
				case EOB_ACT_LAST_MATCH:
					/* This happens because yy_g_n_b()
					 * sees that we've accumulated a
					 * token and flags that we need to
					 * try matching the token before
					 * proceeding.  But for input(),
					 * there's no matching to consider.
					 * So convert the EOB_ACT_LAST_MATCH
					 * to EOB_ACT_END_OF_FILE.
					 */

					/* Reset buffer status. */
					verilog_restart(verilog_in );

					/*FALLTHROUGH*/

				case EOB_ACT_END_OF_FILE:
					{
					if ( verilog_wrap( ) )
						return EOF;

					if ( ! (yy_did_buffer_switch_on_eof) )
						YY_NEW_FILE;
#ifdef __cplusplus
					return yyinput();
#else
					return input();
#endif
					}

				case EOB_ACT_CONTINUE_SCAN:
					(yy_c_buf_p) = (yytext_ptr) + offset;
					break;
				}
			}
		}

	c = *(unsigned char *) (yy_c_buf_p);	/* cast for 8-bit char's */
	*(yy_c_buf_p) = '\0';	/* preserve verilog_text */
	(yy_hold_char) = *++(yy_c_buf_p);

	YY_CURRENT_BUFFER_LVALUE->yy_at_bol = (c == '\n');

	return c;
}
#endif	/* ifndef YY_NO_INPUT */

/** Immediately switch to a different input stream.
 * @param input_file A readable stream.
 * 
 * @note This function does not reset the start condition to @c INITIAL .
 */
    void verilog_restart  (FILE * input_file )
{
    
	if ( ! YY_CURRENT_BUFFER ){
        verilog_ensure_buffer_stack ();
		YY_CURRENT_BUFFER_LVALUE =
            verilog__create_buffer(verilog_in,YY_BUF_SIZE );
	}

	verilog__init_buffer(YY_CURRENT_BUFFER,input_file );
	verilog__load_buffer_state( );
}

/** Switch to a different input buffer.
 * @param new_buffer The new input buffer.
 * 
 */
    void verilog__switch_to_buffer  (YY_BUFFER_STATE  new_buffer )
{
    
	/* TODO. We should be able to replace this entire function body
	 * with
	 *		verilog_pop_buffer_state();
	 *		verilog_push_buffer_state(new_buffer);
     */
	verilog_ensure_buffer_stack ();
	if ( YY_CURRENT_BUFFER == new_buffer )
		return;

	if ( YY_CURRENT_BUFFER )
		{
		/* Flush out information for old buffer. */
		*(yy_c_buf_p) = (yy_hold_char);
		YY_CURRENT_BUFFER_LVALUE->yy_buf_pos = (yy_c_buf_p);
		YY_CURRENT_BUFFER_LVALUE->yy_n_chars = (yy_n_chars);
		}

	YY_CURRENT_BUFFER_LVALUE = new_buffer;
	verilog__load_buffer_state( );

	/* We don't actually know whether we did this switch during
	 * EOF (verilog_wrap()) processing, but the only time this flag
	 * is looked at is after verilog_wrap() is called, so it's safe
	 * to go ahead and always set it.
	 */
	(yy_did_buffer_switch_on_eof) = 1;
}

static void verilog__load_buffer_state  (void)
{
    	(yy_n_chars) = YY_CURRENT_BUFFER_LVALUE->yy_n_chars;
	(yytext_ptr) = (yy_c_buf_p) = YY_CURRENT_BUFFER_LVALUE->yy_buf_pos;
	verilog_in = YY_CURRENT_BUFFER_LVALUE->yy_input_file;
	(yy_hold_char) = *(yy_c_buf_p);
}

/** Allocate and initialize an input buffer state.
 * @param file A readable stream.
 * @param size The character buffer size in bytes. When in doubt, use @c YY_BUF_SIZE.
 * 
 * @return the allocated buffer state.
 */
    YY_BUFFER_STATE verilog__create_buffer  (FILE * file, int  size )
{
	YY_BUFFER_STATE b;
    
	b = (YY_BUFFER_STATE) verilog_alloc(sizeof( struct yy_buffer_state )  );
	if ( ! b )
		YY_FATAL_ERROR( "out of dynamic memory in verilog__create_buffer()" );

	b->yy_buf_size = (yy_size_t)size;

	/* yy_ch_buf has to be 2 characters longer than the size given because
	 * we need to put in 2 end-of-buffer characters.
	 */
	b->yy_ch_buf = (char *) verilog_alloc(b->yy_buf_size + 2  );
	if ( ! b->yy_ch_buf )
		YY_FATAL_ERROR( "out of dynamic memory in verilog__create_buffer()" );

	b->yy_is_our_buffer = 1;

	verilog__init_buffer(b,file );

	return b;
}

/** Destroy the buffer.
 * @param b a buffer created with verilog__create_buffer()
 * 
 */
    void verilog__delete_buffer (YY_BUFFER_STATE  b )
{
    
	if ( ! b )
		return;

	if ( b == YY_CURRENT_BUFFER ) /* Not sure if we should pop here. */
		YY_CURRENT_BUFFER_LVALUE = (YY_BUFFER_STATE) 0;

	if ( b->yy_is_our_buffer )
		verilog_free((void *) b->yy_ch_buf  );

	verilog_free((void *) b  );
}

/* Initializes or reinitializes a buffer.
 * This function is sometimes called more than once on the same buffer,
 * such as during a verilog_restart() or at EOF.
 */
    static void verilog__init_buffer  (YY_BUFFER_STATE  b, FILE * file )

{
	int oerrno = errno;
    
	verilog__flush_buffer(b );

	b->yy_input_file = file;
	b->yy_fill_buffer = 1;

    /* If b is the current buffer, then verilog__init_buffer was _probably_
     * called from verilog_restart() or through yy_get_next_buffer.
     * In that case, we don't want to reset the lineno or column.
     */
    if (b != YY_CURRENT_BUFFER){
        b->yy_bs_lineno = 1;
        b->yy_bs_column = 0;
    }

        b->yy_is_interactive = file ? (isatty( fileno(file) ) > 0) : 0;
    
	errno = oerrno;
}

/** Discard all buffered characters. On the next scan, YY_INPUT will be called.
 * @param b the buffer state to be flushed, usually @c YY_CURRENT_BUFFER.
 * 
 */
    void verilog__flush_buffer (YY_BUFFER_STATE  b )
{
    	if ( ! b )
		return;

	b->yy_n_chars = 0;

	/* We always need two end-of-buffer characters.  The first causes
	 * a transition to the end-of-buffer state.  The second causes
	 * a jam in that state.
	 */
	b->yy_ch_buf[0] = YY_END_OF_BUFFER_CHAR;
	b->yy_ch_buf[1] = YY_END_OF_BUFFER_CHAR;

	b->yy_buf_pos = &b->yy_ch_buf[0];

	b->yy_at_bol = 1;
	b->yy_buffer_status = YY_BUFFER_NEW;

	if ( b == YY_CURRENT_BUFFER )
		verilog__load_buffer_state( );
}

/** Pushes the new state onto the stack. The new state becomes
 *  the current state. This function will allocate the stack
 *  if necessary.
 *  @param new_buffer The new state.
 *  
 */
void verilog_push_buffer_state (YY_BUFFER_STATE new_buffer )
{
    	if (new_buffer == NULL)
		return;

	verilog_ensure_buffer_stack();

	/* This block is copied from verilog__switch_to_buffer. */
	if ( YY_CURRENT_BUFFER )
		{
		/* Flush out information for old buffer. */
		*(yy_c_buf_p) = (yy_hold_char);
		YY_CURRENT_BUFFER_LVALUE->yy_buf_pos = (yy_c_buf_p);
		YY_CURRENT_BUFFER_LVALUE->yy_n_chars = (yy_n_chars);
		}

	/* Only push if top exists. Otherwise, replace top. */
	if (YY_CURRENT_BUFFER)
		(yy_buffer_stack_top)++;
	YY_CURRENT_BUFFER_LVALUE = new_buffer;

	/* copied from verilog__switch_to_buffer. */
	verilog__load_buffer_state( );
	(yy_did_buffer_switch_on_eof) = 1;
}

/** Removes and deletes the top of the stack, if present.
 *  The next element becomes the new top.
 *  
 */
void verilog_pop_buffer_state (void)
{
    	if (!YY_CURRENT_BUFFER)
		return;

	verilog__delete_buffer(YY_CURRENT_BUFFER );
	YY_CURRENT_BUFFER_LVALUE = NULL;
	if ((yy_buffer_stack_top) > 0)
		--(yy_buffer_stack_top);

	if (YY_CURRENT_BUFFER) {
		verilog__load_buffer_state( );
		(yy_did_buffer_switch_on_eof) = 1;
	}
}

/* Allocates the stack if it does not exist.
 *  Guarantees space for at least one push.
 */
static void verilog_ensure_buffer_stack (void)
{
	yy_size_t num_to_alloc;
    
	if (!(yy_buffer_stack)) {

		/* First allocation is just for 2 elements, since we don't know if this
		 * scanner will even need a stack. We use 2 instead of 1 to avoid an
		 * immediate realloc on the next call.
         */
		num_to_alloc = 1; /* After all that talk, this was set to 1 anyways... */
		(yy_buffer_stack) = (struct yy_buffer_state**)verilog_alloc
								(num_to_alloc * sizeof(struct yy_buffer_state*)
								);
		if ( ! (yy_buffer_stack) )
			YY_FATAL_ERROR( "out of dynamic memory in verilog_ensure_buffer_stack()" );
								  
		memset((yy_buffer_stack), 0, num_to_alloc * sizeof(struct yy_buffer_state*));
				
		(yy_buffer_stack_max) = num_to_alloc;
		(yy_buffer_stack_top) = 0;
		return;
	}

	if ((yy_buffer_stack_top) >= ((yy_buffer_stack_max)) - 1){

		/* Increase the buffer to prepare for a possible push. */
		yy_size_t grow_size = 8 /* arbitrary grow size */;

		num_to_alloc = (yy_buffer_stack_max) + grow_size;
		(yy_buffer_stack) = (struct yy_buffer_state**)verilog_realloc
								((yy_buffer_stack),
								num_to_alloc * sizeof(struct yy_buffer_state*)
								);
		if ( ! (yy_buffer_stack) )
			YY_FATAL_ERROR( "out of dynamic memory in verilog_ensure_buffer_stack()" );

		/* zero only the new slots.*/
		memset((yy_buffer_stack) + (yy_buffer_stack_max), 0, grow_size * sizeof(struct yy_buffer_state*));
		(yy_buffer_stack_max) = num_to_alloc;
	}
}

/** Setup the input buffer state to scan directly from a user-specified character buffer.
 * @param base the character buffer
 * @param size the size in bytes of the character buffer
 * 
 * @return the newly allocated buffer state object. 
 */
YY_BUFFER_STATE verilog__scan_buffer  (char * base, yy_size_t  size )
{
	YY_BUFFER_STATE b;
    
	if ( size < 2 ||
	     base[size-2] != YY_END_OF_BUFFER_CHAR ||
	     base[size-1] != YY_END_OF_BUFFER_CHAR )
		/* They forgot to leave room for the EOB's. */
		return 0;

	b = (YY_BUFFER_STATE) verilog_alloc(sizeof( struct yy_buffer_state )  );
	if ( ! b )
		YY_FATAL_ERROR( "out of dynamic memory in verilog__scan_buffer()" );

	b->yy_buf_size = size - 2;	/* "- 2" to take care of EOB's */
	b->yy_buf_pos = b->yy_ch_buf = base;
	b->yy_is_our_buffer = 0;
	b->yy_input_file = 0;
	b->yy_n_chars = b->yy_buf_size;
	b->yy_is_interactive = 0;
	b->yy_at_bol = 1;
	b->yy_fill_buffer = 0;
	b->yy_buffer_status = YY_BUFFER_NEW;

	verilog__switch_to_buffer(b  );

	return b;
}

/** Setup the input buffer state to scan a string. The next call to verilog_lex() will
 * scan from a @e copy of @a str.
 * @param yystr a NUL-terminated string to scan
 * 
 * @return the newly allocated buffer state object.
 * @note If you want to scan bytes that may contain NUL values, then use
 *       verilog__scan_bytes() instead.
 */
YY_BUFFER_STATE verilog__scan_string (yyconst char * yystr )
{
    
	return verilog__scan_bytes(yystr,strlen(yystr) );
}

/** Setup the input buffer state to scan the given bytes. The next call to verilog_lex() will
 * scan from a @e copy of @a bytes.
 * @param yybytes the byte buffer to scan
 * @param _yybytes_len the number of bytes in the buffer pointed to by @a bytes.
 * 
 * @return the newly allocated buffer state object.
 */
YY_BUFFER_STATE verilog__scan_bytes  (yyconst char * yybytes, yy_size_t  _yybytes_len )
{
	YY_BUFFER_STATE b;
	char *buf;
	yy_size_t n;
	yy_size_t i;
    
	/* Get memory for full buffer, including space for trailing EOB's. */
	n = _yybytes_len + 2;
	buf = (char *) verilog_alloc(n  );
	if ( ! buf )
		YY_FATAL_ERROR( "out of dynamic memory in verilog__scan_bytes()" );

	for ( i = 0; i < _yybytes_len; ++i )
		buf[i] = yybytes[i];

	buf[_yybytes_len] = buf[_yybytes_len+1] = YY_END_OF_BUFFER_CHAR;

	b = verilog__scan_buffer(buf,n );
	if ( ! b )
		YY_FATAL_ERROR( "bad buffer in verilog__scan_bytes()" );

	/* It's okay to grow etc. this buffer, and we should throw it
	 * away when we're done.
	 */
	b->yy_is_our_buffer = 1;

	return b;
}

#ifndef YY_EXIT_FAILURE
#define YY_EXIT_FAILURE 2
#endif

static void yy_fatal_error (yyconst char* msg )
{
			(void) fprintf( stderr, "%s\n", msg );
	exit( YY_EXIT_FAILURE );
}

/* Redefine yyless() so it works in section 3 code. */

#undef yyless
#define yyless(n) \
	do \
		{ \
		/* Undo effects of setting up verilog_text. */ \
        int yyless_macro_arg = (n); \
        YY_LESS_LINENO(yyless_macro_arg);\
		verilog_text[verilog_leng] = (yy_hold_char); \
		(yy_c_buf_p) = verilog_text + yyless_macro_arg; \
		(yy_hold_char) = *(yy_c_buf_p); \
		*(yy_c_buf_p) = '\0'; \
		verilog_leng = yyless_macro_arg; \
		} \
	while ( 0 )

/* Accessor  methods (get/set functions) to struct members. */

/** Get the current line number.
 * 
 */
int verilog_get_lineno  (void)
{
        
    return verilog_lineno;
}

/** Get the input stream.
 * 
 */
FILE *verilog_get_in  (void)
{
        return verilog_in;
}

/** Get the output stream.
 * 
 */
FILE *verilog_get_out  (void)
{
        return verilog_out;
}

/** Get the length of the current token.
 * 
 */
yy_size_t verilog_get_leng  (void)
{
        return verilog_leng;
}

/** Get the current token.
 * 
 */

char *verilog_get_text  (void)
{
        return verilog_text;
}

/** Set the current line number.
 * @param _line_number line number
 * 
 */
void verilog_set_lineno (int  _line_number )
{
    
    verilog_lineno = _line_number;
}

/** Set the input stream. This does not discard the current
 * input buffer.
 * @param _in_str A readable stream.
 * 
 * @see verilog__switch_to_buffer
 */
void verilog_set_in (FILE *  _in_str )
{
        verilog_in = _in_str ;
}

void verilog_set_out (FILE *  _out_str )
{
        verilog_out = _out_str ;
}

int verilog_get_debug  (void)
{
        return verilog__flex_debug;
}

void verilog_set_debug (int  _bdebug )
{
        verilog__flex_debug = _bdebug ;
}

static int yy_init_globals (void)
{
        /* Initialization is the same as for the non-reentrant scanner.
     * This function is called from verilog_lex_destroy(), so don't allocate here.
     */

    (yy_buffer_stack) = 0;
    (yy_buffer_stack_top) = 0;
    (yy_buffer_stack_max) = 0;
    (yy_c_buf_p) = (char *) 0;
    (yy_init) = 0;
    (yy_start) = 0;

/* Defined in main.c */
#ifdef YY_STDINIT
    verilog_in = stdin;
    verilog_out = stdout;
#else
    verilog_in = (FILE *) 0;
    verilog_out = (FILE *) 0;
#endif

    /* For future reference: Set errno on error, since we are called by
     * verilog_lex_init()
     */
    return 0;
}

/* verilog_lex_destroy is for both reentrant and non-reentrant scanners. */
int verilog_lex_destroy  (void)
{
    
    /* Pop the buffer stack, destroying each element. */
	while(YY_CURRENT_BUFFER){
		verilog__delete_buffer(YY_CURRENT_BUFFER  );
		YY_CURRENT_BUFFER_LVALUE = NULL;
		verilog_pop_buffer_state();
	}

	/* Destroy the stack itself. */
	verilog_free((yy_buffer_stack) );
	(yy_buffer_stack) = NULL;

    /* Reset the globals. This is important in a non-reentrant scanner so the next time
     * verilog_lex() is called, initialization will occur. */
    yy_init_globals( );

    return 0;
}

/*
 * Internal utility routines.
 */

#ifndef yytext_ptr
static void yy_flex_strncpy (char* s1, yyconst char * s2, int n )
{
		
	int i;
	for ( i = 0; i < n; ++i )
		s1[i] = s2[i];
}
#endif

#ifdef YY_NEED_STRLEN
static int yy_flex_strlen (yyconst char * s )
{
	int n;
	for ( n = 0; s[n]; ++n )
		;

	return n;
}
#endif

void *verilog_alloc (yy_size_t  size )
{
			return (void *) malloc( size );
}

void *verilog_realloc  (void * ptr, yy_size_t  size )
{
		
	/* The cast to (char *) in the following accommodates both
	 * implementations that use char* generic pointers, and those
	 * that use void* generic pointers.  It works with the latter
	 * because both ANSI C and C++ allow castless assignment from
	 * any pointer type to void*, and deal with argument conversions
	 * as though doing an assignment.
	 */
	return (void *) realloc( (char *) ptr, size );
}

void verilog_free (void * ptr )
{
			free( (char *) ptr );	/* see verilog_realloc() for (char *) cast */
}

#define YYTABLES_NAME "yytables"

#line 406 "verilog.l"



void lex_start_table()
{
  BEGIN(UDPTABLE);
}
void lex_end_table()
{
  BEGIN(INITIAL);
}



static int yyerror()
{
  return 1;
}
int verilog_wrap()
{
  return 1;
}
static void reset_lexor()
{
  verilog_restart(verilog_input);
  line =1;
  file =verilog_file;
}
static void line_directive()
{
  assert(strncmp(verilog_text,"`line",5) == 0);
  char*cp = verilog_text + strlen("`line");
  cp += strspn(cp, " ");
  line = strtoul(cp,&cp,10);

  cp += strspn(cp, " ");
  if (*cp == 0) return;
  char*qt1 = strchr(verilog_text, '"');
  assert(qt1);
  qt1 += 1;
  char*qt2 = strchr(qt1, '"');
  assert(qt2);
  file.assign(qt1,(qt2-qt1));
}


