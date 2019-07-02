/*
 * File:	generator.cpp
 *
 * Description:	This file contains the public and member function
 *		definitions for the code generator for Simple C.
 *
 *		Extra functionality:
 *		- putting all the global declarations at the end
 */

# include <vector>
# include <iostream>
# include <sstream>
# include <string>
# include <map>
# include "generator.h"
# include "Register.h"
# include "machine.h"
# include "Tree.h"
# include "assert.h"

using namespace std;

static int offset;
static string funcname;
int temp_offset;
map<string, Label> string_labels;

/* This needs to be zero for the next phase. */

# define SIMPLE_PROLOGUE 0


/* This shouid be set if we want to use the callee-saved registers. */

# define CALLEE_SAVED 0


/* The registers and their related functions */

typedef vector<Register *>Registers;
static Register *rax = new Register("%rax", "%eax", "%al");
static Register *rbx = new Register("%rbx", "%ebx", "%bl");
static Register *rcx = new Register("%rcx", "%ecx", "%cl");
static Register *rdx = new Register("%rdx", "%edx", "%dl");
static Register *rsi = new Register("%rsi", "%esi", "%sil");
static Register *rdi = new Register("%rdi", "%edi", "%dil");
static Register *r8 = new Register("%r8", "%r8d", "%r8b");
static Register *r9 = new Register("%r9", "%r9d", "%r9b");
static Register *r10 = new Register("%r10", "%r10d", "%r10b");
static Register *r11 = new Register("%r11", "%r11d", "%r11b");
static Register *r12 = new Register("%r12", "%r12d", "%r12b");
static Register *r13 = new Register("%r13", "%r13d", "%r13b");
static Register *r14 = new Register("%r14", "%r14d", "%r14b");
static Register *r15 = new Register("%r15", "%r15d", "%r15b");

static Registers registers;
static Registers parameters = {rdi, rsi, rdx, rcx, r8, r9};
//static Registers caller_saved = {rax, rdi, rsi, rdx, rcx, r8, r9, r10, r11};
static Registers caller_saved = {r11, r10, r9, r8, rcx, rdx, rsi, rdi, rax};

# if CALLEE_SAVED
static Registers callee_saved = {rbx, r12, r13, r14, r15};
# else
static Registers callee_saved = {};
# endif

static void assign(Expression *expr, Register *reg);
static void load(Expression *expr, Register *reg);


/*
 * Function:	suffix (private)
 *
 * Description:	Return the suffix for an opcode based on the given size.
 */


static string suffix_size(unsigned long size)
{
    return size == 1 ? "b" : (size == 4 ? "l" : "q");
}


static string suffix(const Expression *expr) {
    return expr->type().size() == 4 ? "l\t" : "q\t";
}


/*
 * Function:	suffix (private)
 *
 * Description:	Return the suffix for an opcode based on the size of the
 *		given expression.
 */
/*
static string suffix(Expression *expr)
{
    return suffix(expr->type().size());
}
*/

/*
 * Function:	align (private)
 *
 * Description:	Return the number of bytes necessary to align the given
 *		offset on the stack.
 */

static int align(int offset)
{
    if (offset % STACK_ALIGNMENT == 0)
	return 0;

    return STACK_ALIGNMENT - (abs(offset) % STACK_ALIGNMENT);
}


/*
 * Function:	operator << (private)
 *
 * Description:	Write an expression as an operand to the specified stream.
 *		This function first checks to see if the expression is in a
 *		register, and if not then uses its offset.
 */

static ostream &operator <<(ostream &ostr, Expression *expr)
{
    if (expr->_register != nullptr)
	return ostr << expr->_register;

    expr->operand(ostr);
    return ostr;
}


/*
 * Function:	Expression::operand
 *
 * Description:	Write an expression as an operand to the specified stream.
 */

void Expression::operand(ostream &ostr) const
{
    ostr << _offset << "(%rbp)";
}


/*
 * Function:	Identifier::operand
 *
 * Description:	Write an identifier as an operand to the specified stream.
 */

void Identifier::operand(ostream &ostr) const
{
    if (_symbol->_offset == 0)
	ostr << global_prefix << _symbol->name() << global_suffix;
    else
	ostr << _symbol->_offset << "(%rbp)";
}


/*
 * Function:	Number::operand
 *
 * Description:	Write a number as an operand to the specified stream.
 */

void Number::operand(ostream &ostr) const
{
    ostr << "$" << _value;
}


/*
 * Function:	Call::generate
 *
 * Description:	Generate code for a function call expression.
 */

void Call::generate()
{
    unsigned long value, size, bytesPushed = 0;


    /* Generate any arguments with function calls first. */

    for (int i = _args.size() - 1; i >= 0; i --)
	if (_args[i]->_hasCall)
	    _args[i]->generate();

    //for(unsigned int i = 0; i < registers.size(); i++) {
    //    load(nullptr, registers[i]);
    //}
    
    /* Adjust the stack if necessary. */

    if (_args.size() > NUM_PARAM_REGS) {
	bytesPushed = align((_args.size() - NUM_PARAM_REGS) * SIZEOF_PARAM);

	if (bytesPushed > 0)
	    cout << "\tsubq\t$" << bytesPushed << ", %rsp" << endl;
    }


    /* Move the arguments into the correct registers or memory locations. */

    for (int i = _args.size() - 1; i >= 0; i --) {
	    size = _args[i]->type().size();

	    if (!_args[i]->_hasCall)
	        _args[i]->generate();

	    if (i < NUM_PARAM_REGS)
	        load(_args[i], parameters[i]);

	    else {
	        bytesPushed += SIZEOF_PARAM;

            if (_args[i]->_register)
                cout << "\tpushq\t" << _args[i]->_register->name() << endl;
            else if (_args[i]->isNumber(value) || size == SIZEOF_PARAM)
                cout << "\tpushq\t" << _args[i] << endl;
            else {
                load(_args[i], rax);
                cout << "\tpushq\t%rax" << endl;
	        }
	    }

	    assign(_args[i], nullptr);
    }


    /* Spill any caller-saved registers still in use. */

    for (unsigned i = 0; i < caller_saved.size(); i ++)
	    load(nullptr, caller_saved[i]);


    /* Call the function.  Technically, we only need to assign the number
       of floating point arguments to %eax if the function being called
       takes a variable number of arguments.  But, it never hurts. */

    if (_id->type().parameters() == nullptr)
	cout << "\tmovl\t$0, %eax" << endl;

    cout << "\tcall\t" << global_prefix << _id->name() << endl;


    /* Reclaim the space of any arguments pushed on the stack. */

    if (bytesPushed > 0)
	cout << "\taddq\t$" << bytesPushed << ", %rsp" << endl;

    assign(this, rax);
}


/*
 * Function:	Block::generate
 *
 * Description:	Generate code for this block, which simply means we
 *		generate code for each statement within the block.
 */

void Block::generate()
{
    for (unsigned i = 0; i < _stmts.size(); i ++)
	    _stmts[i]->generate();
    
}


/*
 * Function:	Simple::generate
 *
 * Description:	Generate code for a simple (expression) statement, which
 *		means simply generating code for the expression.
 */

void Simple::generate()
{
    _expr->generate();
    assign(_expr, nullptr);
}


/*
 * Function:	Function::generate
 *
 * Description:	Generate code for this function, which entails allocating
 *		space for local variables, then emitting our prologue, the
 *		body of the function, and the epilogue.
 *
 *		The stack must be aligned at the point at which a function
 *		begins execution.  Since the call instruction pushes the
 *		return address on the stack and each function is expected
 *		to push its base pointer, we adjust our offset by that
 *		amount and then perform the alignment.
 *
 *		On a 32-bit Intel platform, 8 bytes are pushed (4 for the
 *		return address and 4 for the base pointer).  Since Linux
 *		requires a 4-byte alignment, all we need to do is ensure
 *		the stack size is a multiple of 4, which will usually
 *		already be the case.  However, since OS X requires a
 *		16-byte alignment (thanks, Apple, for inventing your own
 *		standards), we will often see an extra amount of stack
 *		space allocated.
 *
 *		On a 64-bit Intel platform, 16 bytes are pushed (8 for the
 *		return address and 8 for the base pointer).  Both Linux and
 *		OS X require 16-byte alignment.
 */

void Function::generate()
{
# if 1
    unsigned size;
    int param_offset;
    Parameters *params = _id->type().parameters();
    const Symbols &symbols = _body->declarations()->symbols();

    /* Assign offsets to all symbols within the scope of the function. */

    param_offset = PARAM_OFFSET + SIZEOF_REG * callee_saved.size();
    offset = param_offset;
    allocate(offset);


    /* Generate the prologue. */

    funcname = _id->name();
    cout << global_prefix << funcname << ":" << endl;
    cout << "\tpushq\t%rbp" << endl;

    for (unsigned i = 0; i < callee_saved.size(); i ++)
	    cout << "\tpushq\t" << callee_saved[i] << endl;

    cout << "\tmovq\t%rsp, %rbp" << endl;

    if (SIMPLE_PROLOGUE) {
	    offset -= align(offset - param_offset);
	    cout << "\tsubq\t$" << -offset << ", %rsp" << endl;
    } else {
	    cout << "\tmovl\t$" << funcname << ".size, %eax" << endl;
	    cout << "\tsubq\t%rax, %rsp" << endl;
    }


    /* Spill any parameters. */

    for (unsigned i = 0; i < NUM_PARAM_REGS; i ++)
	if (i < params->size()) {
	    size = symbols[i]->type().size();
	    cout << "\tmov" << suffix_size(size) << "\t" << parameters[i]->name(size);
	    cout << ", " << symbols[i]->_offset << "(%rbp)" << endl;
	} else
	    break;


    /* Generate the body and epilogue. */

    registers = (_hasCall && callee_saved.size() ? callee_saved : caller_saved);
    _body->generate();

    cout << endl << global_prefix << funcname << ".exit:" << endl;
    cout << "\tmovq\t%rbp, %rsp" << endl;

    for (int i = callee_saved.size() - 1; i >= 0; i --)
	cout << "\tpopq\t" << callee_saved[i] << endl;

    cout << "\tpopq\t%rbp" << endl;
    cout << "\tret" << endl << endl;


    /* Finish aligning the stack. */

    if (!SIMPLE_PROLOGUE) {
	offset -= align(offset - param_offset);
	cout << "\t.set\t" << funcname << ".size, " << -offset << endl;
    }

    cout << "\t.globl\t" << global_prefix << funcname << endl << endl;

# else

    /* This is really all the students need to do. */

    offset = 0;
    Parameters *params = _id->type().parameters();
    const Symbols &symbols = _body->declarations()->symbols();
    string parameters[] = {"%edi", "%esi", "%edx", "%ecx", "%r8d", "%r9d"};
    
    for (unsigned i = 0; i < symbols.size(); i ++)
	if (i < 6 || i >= params->size()) {
	    offset -= symbols[i]->type().size();
	    symbols[i]->_offset = offset;
	} else
	    symbols[i]->_offset = 16 + 8 * (i - 6);

    while (offset % 16 != 0)
	offset --;

    cout << global_prefix << _id->name() << ":" << endl;
    cout << "\tpushq\t%rbp" << endl;
    cout << "\tmovq\t%rsp, %rbp" << endl;
    cout << "\tsubq\t$" << -offset << ", %rsp" << endl;

    for (unsigned i = 0; i < 6; i ++)
	if (i < params->size()) {
	    cout << "\tmovl\t" << parameters[i] << ", ";
	    cout << symbols[i]->_offset << "(%rbp)" << endl;
	} else
	    break;

    _body->generate();

    cout << "\tmovq\t%rbp, %rsp" << endl;
    cout << "\tpopq\t%rbp" << endl;
    cout << "\tret" << endl << endl;
    cout << "\t.globl\t" << global_prefix << _id->name() << endl << endl;
# endif
}


/*
 * Function:	generateGlobals
 *
 * Description:	Generate code for any global variable declarations.
 */

void generateGlobals(Scope *scope)
{
    const Symbols &symbols = scope->symbols();
    map<string, Label>::iterator it;
    
    for (unsigned i = 0; i < symbols.size(); i ++)
	if (!symbols[i]->type().isFunction()) {
	    cout << "\t.comm\t" << global_prefix << symbols[i]->name() << ", ";
	    cout << symbols[i]->type().size() << endl;
	}
    
   for (it = string_labels.begin(); it != string_labels.end(); it++)
        cout << it->second << ":\t.asciz\t" << it->first << endl;

}

/*
 * Function:	Assignment::generate
 *
 * Description:	Generate code for an assignment statement.
 *
 *		NOT FINISHED: Only works if the right-hand side is an
 *		integer literal and the left-hand side is an integer
 *		scalar.
 */

void Assignment::generate() {
    cout << "#assignment start" << endl;
    
    Expression *p;
    stringstream ss;
    
    if(_left->isDereference(p) == false)
        _left->generate();
    else
        p->generate();
    
    _right->generate();

    if (_right->_register == nullptr)
        load(_right, getreg());

    if(_left->isDereference(p) == false) {
        ss << _left;
    } else {
        if (p->_register == nullptr)
            load(p, getreg());
        ss << "(" << p << ")";
        assign(p, nullptr);
    }
    
    assign(_right, nullptr);
    cout << "\tmov" << suffix(_right) << _right << ", " << ss.str() << endl;
    cout << "#assignment end" << endl;
}

/*
 * Function:	assign (private)
 *
 * Description:	Assign the given expression to the given register.  No
 *		assembly code is generated here as only the pointers are
 *		updated.
 *
 *		NOT FINISHED: Needs to be finished with code from class.
 */

static void assign(Expression *expr, Register *reg)
{
    if (expr != nullptr) {
        if (expr->_register != nullptr)
            expr->_register->_node = nullptr;

        expr->_register = reg;
    }

    if (reg != nullptr) {
        if (reg->_node != nullptr)
            reg->_node->_register = nullptr;

        reg->_node = expr;
    }
}

/*
 * Function:	load (private)
 *
 * Description:	Load the given expression into the given register.
 *
 */


static void load(Expression *expr, Register *reg) {
    if (reg->_node != expr) { // sanity check

        if (reg->_node != nullptr) {
            unsigned size = reg->_node->type().size();
            offset -= size;
            reg->_node->_offset = offset;

            //temp_offset = temp_offset - size;
            //reg->_node->_offset = temp_offset;
            cout << "\tmov" << suffix(reg->_node); // spill instead of failing
            cout << reg->name(size) << ", ";
            cout << offset << "(%rbp)" << "# spill" << endl ;
        }

        if (expr != nullptr) {
            unsigned size = expr->type().size();
            // use suffix(x) to return the correct opcode suffix for x
            cout << "\tmov" << suffix(expr) << expr;
            cout << ", " << reg->name(size) << endl;
        }

        assign(expr, reg);
    }
}

Register *getreg() {
    for (unsigned i = 0; i < registers.size(); i++)
        if (registers[i]->_node == nullptr) {
            return registers[i];
        }
    
    load(nullptr, registers[0]);    // spill the first register so it's available
    return registers[0];
}

static void compute(Expression *result, Expression *left, Expression *right, const string &opcode) {
    left->generate();
    right->generate();

    if (left->_register == nullptr)
        load(left, getreg());

    cout << "\t" << opcode << suffix(left);
    cout << right << ", " << left << endl;

    assign(right, nullptr);
    assign(result, left->_register);
}

void Add::generate() {
    cout << "#add start" << endl;
    compute(this, _left, _right, "add");
    cout << "#add end" << endl;
}

void Subtract::generate() {
    compute(this, _left, _right, "sub");
}

void Multiply::generate() {
    compute(this, _left, _right, "imul");
}

void Divide::generate() {
    _left->generate();
    _right->generate();

    load(_left, rax);
    load(nullptr, rdx);
    load(_right, rcx);
    
    if (_right->type().size() == 4) {
        cout << "\tcltd" << endl;
        cout << "\tidivl\t" << _right << endl;
    } else {
        cout << "\tcqto" << endl;
        cout << "\tidivq\t" << _right << endl;
    }
    
    assign(_right, nullptr);
    assign(this, rax);          // rax holds quotient
    assign(nullptr, rdx);

}


void Remainder::generate() {
    _left->generate();
    _right->generate();

    load(nullptr, rax);
    load(_left, rdx);
    load(_right, rcx);
    
    if (_right->type().size() == 4) {
        cout << "\tcltd" << endl;
        cout << "\tidivl\t" << _right << endl;
    } else {
        cout << "\tcqto" << endl;
        cout << "\tidivq\t" << _right << endl;
    }
    
    assign(_right, nullptr);
    assign(nullptr, rax);
    assign(this, rdx);             // rdx holds remainder

}

void Negate::generate() {
    _expr->generate();
    load(_expr, getreg());
    cout << "\tneg" << suffix_size(_type.size()) << "\t" << _expr << endl;

    assign(this, _expr->_register);
}

void Not::generate() {
    _expr->generate();
    load(_expr, getreg());
    cout << "\tcmp" << suffix_size(_expr->type().size()) << "\t$0," << _expr << endl;
    cout << "\tsete\t" << _expr->_register->name(1) << endl;
    cout << "\tmovzbl\t" << _expr->_register->name(1) << ", " << _expr->_register->name(4) << endl;

    assign(this, _expr->_register);
}

void Cast::generate()
{
    _expr->generate();
    int source = _expr->type().size();
    int target = _type.size();
    
    if (target <= source) {
        if (_expr->_register == nullptr)
            load(_expr, getreg());
        assign(this, _expr->_register);
    } else {
        Register* tmp = getreg();
        load(_expr, tmp);
        cout << "\tmovs" << suffix_size(source) << suffix_size(target) << "\t" << _expr << ", " << tmp->name(target) << endl;
        assign(this, tmp);
    }
}

void LessThan::generate() {
    cout << "#less than start" << endl;
    _left->generate();
    _right->generate();

    if(_left->_register == nullptr)
        load(_left, getreg());

    cout << "\tcmp" << suffix(_right) << _right << ", " << _left << endl;
    cout << "\tsetl\t" << _left->_register->name(1) << endl;
    cout << "\tmovzbl\t" << _left->_register->name(1) << ", " << _left->_register->name(4) << endl;

    assign(this, _left->_register);
    assign(_right, nullptr);
    cout << "#less than end" << endl;
}

void GreaterThan::generate() {
    _left->generate();
    _right->generate();

    if(_left->_register == nullptr)
        load(_left, getreg());

    cout << "\tcmp" << suffix(_right) << _right << ", " << _left << endl;
    cout << "\tsetg\t" << _left->_register->name(1) << endl;
    cout << "\tmovzbl\t" << _left->_register->name(1) << ", " << _left->_register->name(4) << endl;

    assign(this, _left->_register);
    assign(_right, nullptr);
}

void LessOrEqual::generate() {
    _left->generate();
    _right->generate();

    if(_left->_register == nullptr)
        load(_left, getreg());

    cout << "\tcmp" << suffix(_right) << _right << ", " << _left << endl;
    cout << "\tsetle\t" << _left->_register->name(1) << endl;
    cout << "\tmovzbl\t" << _left->_register->name(1) << ", " << _left->_register->name(4) << endl;

    assign(this, _left->_register);
    assign(_right, nullptr);
}

void GreaterOrEqual::generate() {
    _left->generate();
    _right->generate();

    if(_left->_register == nullptr)
        load(_left, getreg());

    cout << "\tcmp" << suffix(_right) << _right << ", " << _left << endl;
    cout << "\tsetge\t" << _left->_register->name(1) << endl;
    cout << "\tmovzbl\t" << _left->_register->name(1) << ", " << _left->_register->name(4) << endl;

    assign(this, _left->_register);
    assign(_right, nullptr);
}

void Equal::generate() {
    _left->generate();
    _right->generate();

    if(_left->_register == nullptr)
        load(_left, getreg());

    cout << "\tcmp\t" << _right << ", " << _left << endl;
    cout << "\tsete\t" << _left->_register->name(1) <<  endl;
    cout << "\tmovzbl\t" << _left->_register->name(1) << ", " <<_left->_register->name(4) << endl;

    assign(this, _left->_register);
    assign(_right, nullptr);
}

void NotEqual::generate() {
    _left->generate();
    _right->generate();

    if(_left->_register == nullptr)
        load(_left, getreg());

    cout << "\tcmp\t" << _right << ", " << _left << endl;
    cout << "\tsetne\t" << _left->_register->name(1) <<  endl;
    cout << "\tmovzbl\t" << _left->_register->name(1) << ", " <<_left->_register->name(4) << endl;
    
    assign(this, _left->_register);
    assign(_right, nullptr);
}

void Expression::test(const Label &label, bool ifTrue) {
    generate();
    if (_register == nullptr)
        load(this, getreg());

    cout << "\tcmp" << suffix(this) << "$0, " << this << endl;
    cout << (ifTrue ? "\tjne\t" : "\tje\t") << label << endl;

    assign(this, nullptr);        // done
}

/*
logical or
-----------------
if (E1 != 0)
    result = 1;
else if (E2 != 0)
    result = 1;
else
    result = 0;
*/

void LogicalOr::generate() {
    cout << "#logical or start" << endl;
    Label L1, L2;

    _left->test(L1, true);
    _right->test(L1, true);

    Register* tmp = getreg();
    
    cout << "\tmov\t $0, " << tmp << endl;
    cout << "\tjmp\t" << L2 << endl;
    cout << L1 << ":" << endl;
    cout << "\tmov\t $1, " << tmp << endl;
    cout << L2 << ":" << endl;

    assign(this, tmp);
    cout << "#logical or end" << endl;
}

void LogicalAnd::generate() {
    Label L1, L2;

    _left->test(L1, false);
    _right->test(L1, false);

    Register* tmp = getreg();
    
    cout << "\tmov\t $0, " << tmp << endl;
    cout << "\tjmp\t" << L2 << endl;
    cout << L1 << ":" << endl;
    cout << "\tmov\t $1, " << tmp << endl;
    cout << L2 << ":" << endl;

    assign(this, tmp);
}

void Return::generate() {
    cout << "#return start" << endl;
    
    _expr->generate();
    load(_expr, rax);
    
    cout << "\tjmp\t" << global_prefix << funcname << ".exit" << endl;
    
    assign(_expr, nullptr);
    cout << "#return end" << endl;
}

// check if expr’s value is 1: if so jump to else
// generate code for if statement and jump to exit
// print else label, generate else statement code (if exists)
// print exit label

void If::generate() {
    cout << "#if start" << endl;

    Label next, exit;

    _expr->test(next, false);
    _thenStmt->generate();

    if(_elseStmt == nullptr)
        cout << next << ":" << endl;
    else {
        cout << "\tjmp\t" << exit << endl;
        cout << next << ":" << endl;
        _elseStmt->generate();
        cout << exit << ":" << endl;
    }

    cout << "#if end" << endl;
}

void While::generate() {
    Label loop, exit;
    cout << loop << ":" << endl;
    
    _expr->test(exit, false);
    _stmt->generate();
    
    cout << "\tjmp\t" << loop << endl;
    cout << exit << ":" << endl;
}

void Address::generate() {
    cout << "#address start" << endl;

    if(isDereference(_expr) != false) {
        _expr->generate();
        assign(this, _expr->_register);
        return;
    }

    _expr->generate();
 
    if(_expr->_register != nullptr)
        assign(this, _expr->_register);
    
    assign(this, getreg());
    cout << "\tleaq\t" << _expr << ", " << this << endl;

    cout << "#address end" << endl;
}

void Dereference::generate()
{
    cout << "#deref start" << endl;
    _expr->generate();

    if(_expr->_register == nullptr)
        load(_expr, getreg());

    cout<< "\tmov" << suffix_size(_type.size()) << "\t(" << _expr << "), " << _expr->_register->name(_type.size()) << endl;
    
    assign(this, _expr->_register);
    
    cout << "#deref end" << endl;
}

void String::operand(ostream &ostr) const{
    if (string_labels.find(_value) == string_labels.end()) {
        Label string_label;
        string_labels[_value] = string_label;
    }
    
    ostr << string_labels[_value];
}
