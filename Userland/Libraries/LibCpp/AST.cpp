/*
 * Copyright (c) 2021, Itamar S. <itamar8910@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include "AST.h"

namespace Cpp {

static void print_indent(FILE* output, int indent)
{
    for (int i = 0; i < indent * 2; ++i)
        out(output, " ");
}

void ASTNode::dump(FILE* output, size_t indent) const
{
    print_indent(output, indent);
    outln(output, "{}[{}:{}->{}:{}]", class_name(), start().line, start().column, end().line, end().column);
}

void TranslationUnit::dump(FILE* output, size_t indent) const
{
    ASTNode::dump(output, indent);
    for (const auto& child : m_declarations) {
        child.dump(output, indent + 1);
    }
}

void FunctionDeclaration::dump(FILE* output, size_t indent) const
{
    ASTNode::dump(output, indent);

    String qualifiers_string;
    if (!m_qualifiers.is_empty()) {
        print_indent(output, indent + 1);
        outln(output, "[{}]", String::join(" ", m_qualifiers));
    }

    m_return_type->dump(output, indent + 1);
    if (!m_name.is_null()) {
        print_indent(output, indent + 1);
        outln(output, "{}", m_name);
    }
    print_indent(output, indent + 1);
    outln(output, "(");
    for (const auto& arg : m_parameters) {
        arg.dump(output, indent + 1);
    }
    print_indent(output, indent + 1);
    outln(output, ")");
    if (!m_definition.is_null()) {
        m_definition->dump(output, indent + 1);
    }
}

NonnullRefPtrVector<Declaration> FunctionDeclaration::declarations() const
{
    NonnullRefPtrVector<Declaration> declarations;
    for (auto& arg : m_parameters) {
        declarations.append(arg);
    }

    if (m_definition)
        declarations.append(m_definition->declarations());

    return declarations;
}

void Type::dump(FILE* output, size_t indent) const
{
    ASTNode::dump(output, indent);
    print_indent(output, indent + 1);
    outln(output, "{}", to_string());
}

String Type::to_string() const
{
    String qualifiers_string;
    if (!m_qualifiers.is_empty())
        qualifiers_string = String::formatted("[{}] ", String::join(" ", m_qualifiers));

    String name;
    if (m_is_auto)
        name = "auto";
    else
        name = m_name.is_null() ? "" : m_name->full_name();

    return String::formatted("{}{}", qualifiers_string, name);
}

String Pointer::to_string() const
{
    if (!m_pointee)
        return {};
    StringBuilder builder;
    builder.append(m_pointee->to_string());
    builder.append("*");
    return builder.to_string();
}

void Parameter::dump(FILE* output, size_t indent) const
{
    ASTNode::dump(output, indent);
    if (m_is_ellipsis) {
        print_indent(output, indent + 1);
        outln(output, "...");
    }
    if (!m_name.is_null()) {
        print_indent(output, indent);
        outln(output, "{}", m_name);
    }
    if (m_type)
        m_type->dump(output, indent + 1);
}

void FunctionDefinition::dump(FILE* output, size_t indent) const
{
    ASTNode::dump(output, indent);
    print_indent(output, indent);
    outln(output, "{{");
    for (const auto& statement : m_statements) {
        statement.dump(output, indent + 1);
    }
    print_indent(output, indent);
    outln(output, "}}");
}

NonnullRefPtrVector<Declaration> FunctionDefinition::declarations() const
{
    NonnullRefPtrVector<Declaration> declarations;
    for (auto& statement : m_statements) {
        declarations.append(statement.declarations());
    }
    return declarations;
}

void VariableDeclaration::dump(FILE* output, size_t indent) const
{
    ASTNode::dump(output, indent);
    if (m_type)
        m_type->dump(output, indent + 1);
    print_indent(output, indent + 1);
    outln(output, "{}", m_name);
    if (m_initial_value)
        m_initial_value->dump(output, indent + 1);
}

void Identifier::dump(FILE* output, size_t indent) const
{
    ASTNode::dump(output, indent);
    print_indent(output, indent);
    outln(output, "{}", m_name);
}

void NumericLiteral::dump(FILE* output, size_t indent) const
{
    ASTNode::dump(output, indent);
    print_indent(output, indent);
    outln(output, "{}", m_value);
}

void BinaryExpression::dump(FILE* output, size_t indent) const
{
    ASTNode::dump(output, indent);

    const char* op_string = nullptr;
    switch (m_op) {
    case BinaryOp::Addition:
        op_string = "+";
        break;
    case BinaryOp::Subtraction:
        op_string = "-";
        break;
    case BinaryOp::Multiplication:
        op_string = "*";
        break;
    case BinaryOp::Division:
        op_string = "/";
        break;
    case BinaryOp::Modulo:
        op_string = "%";
        break;
    case BinaryOp::GreaterThan:
        op_string = ">";
        break;
    case BinaryOp::GreaterThanEquals:
        op_string = ">=";
        break;
    case BinaryOp::LessThan:
        op_string = "<";
        break;
    case BinaryOp::LessThanEquals:
        op_string = "<=";
        break;
    case BinaryOp::BitwiseAnd:
        op_string = "&";
        break;
    case BinaryOp::BitwiseOr:
        op_string = "|";
        break;
    case BinaryOp::BitwiseXor:
        op_string = "^";
        break;
    case BinaryOp::LeftShift:
        op_string = "<<";
        break;
    case BinaryOp::RightShift:
        op_string = ">>";
        break;
    case BinaryOp::EqualsEquals:
        op_string = "==";
        break;
    case BinaryOp::NotEqual:
        op_string = "!=";
        break;
    case BinaryOp::LogicalOr:
        op_string = "||";
        break;
    case BinaryOp::LogicalAnd:
        op_string = "&&";
        break;
    case BinaryOp::Arrow:
        op_string = "->";
        break;
    }

    m_lhs->dump(output, indent + 1);
    print_indent(output, indent + 1);
    VERIFY(op_string);
    outln(output, "{}", op_string);
    m_rhs->dump(output, indent + 1);
}

void AssignmentExpression::dump(FILE* output, size_t indent) const
{
    ASTNode::dump(output, indent);

    const char* op_string = nullptr;
    switch (m_op) {
    case AssignmentOp::Assignment:
        op_string = "=";
        break;
    case AssignmentOp::AdditionAssignment:
        op_string = "+=";
        break;
    case AssignmentOp::SubtractionAssignment:
        op_string = "-=";
        break;
    }

    m_lhs->dump(output, indent + 1);
    print_indent(output, indent + 1);
    VERIFY(op_string);
    outln(output, "{}", op_string);
    m_rhs->dump(output, indent + 1);
}

void FunctionCall::dump(FILE* output, size_t indent) const
{
    ASTNode::dump(output, indent);
    m_callee->dump(output, indent + 1);
    for (const auto& arg : m_arguments) {
        arg.dump(output, indent + 1);
    }
}

void StringLiteral::dump(FILE* output, size_t indent) const
{
    ASTNode::dump(output, indent);
    print_indent(output, indent + 1);
    outln(output, "{}", m_value);
}

void ReturnStatement::dump(FILE* output, size_t indent) const
{
    ASTNode::dump(output, indent);
    if (m_value)
        m_value->dump(output, indent + 1);
}

void EnumDeclaration::dump(FILE* output, size_t indent) const
{
    ASTNode::dump(output, indent);
    print_indent(output, indent);
    outln(output, "{}", m_name);
    for (auto& entry : m_entries) {
        print_indent(output, indent + 1);
        outln(output, "{}", entry);
    }
}

void StructOrClassDeclaration::dump(FILE* output, size_t indent) const
{
    ASTNode::dump(output, indent);
    print_indent(output, indent);
    outln(output, "{}", m_name);
    for (auto& member : m_members) {
        member.dump(output, indent + 1);
    }
}
NonnullRefPtrVector<Declaration> StructOrClassDeclaration::declarations() const
{
    NonnullRefPtrVector<Declaration> declarations;
    for (auto& member : m_members)
        declarations.append(member);
    return declarations;
}

void UnaryExpression::dump(FILE* output, size_t indent) const
{
    ASTNode::dump(output, indent);

    const char* op_string = nullptr;
    switch (m_op) {
    case UnaryOp::BitwiseNot:
        op_string = "~";
        break;
    case UnaryOp::Not:
        op_string = "!";
        break;
    case UnaryOp::Plus:
        op_string = "+";
        break;
    case UnaryOp::Minus:
        op_string = "-";
        break;
    case UnaryOp::PlusPlus:
        op_string = "++";
        break;
    case UnaryOp::Address:
        op_string = "&";
        break;
    default:
        op_string = "<invalid>";
    }

    VERIFY(op_string);
    print_indent(output, indent + 1);
    outln(output, "{}", op_string);
    m_lhs->dump(output, indent + 1);
}

void BooleanLiteral::dump(FILE* output, size_t indent) const
{
    ASTNode::dump(output, indent);
    print_indent(output, indent + 1);
    outln(output, "{}", m_value ? "true" : "false");
}

void Pointer::dump(FILE* output, size_t indent) const
{
    ASTNode::dump(output, indent);
    if (!m_pointee.is_null()) {
        m_pointee->dump(output, indent + 1);
    }
}

void MemberExpression::dump(FILE* output, size_t indent) const
{
    ASTNode::dump(output, indent);
    m_object->dump(output, indent + 1);
    m_property->dump(output, indent + 1);
}

void BlockStatement::dump(FILE* output, size_t indent) const
{
    ASTNode::dump(output, indent);
    for (auto& statement : m_statements) {
        statement.dump(output, indent + 1);
    }
}

void ForStatement::dump(FILE* output, size_t indent) const
{
    ASTNode::dump(output, indent);
    if (m_init)
        m_init->dump(output, indent + 1);
    if (m_test)
        m_test->dump(output, indent + 1);
    if (m_update)
        m_update->dump(output, indent + 1);
    if (m_body)
        m_body->dump(output, indent + 1);
}

NonnullRefPtrVector<Declaration> Statement::declarations() const
{
    if (is_declaration()) {
        NonnullRefPtrVector<Declaration> vec;
        const auto& decl = static_cast<const Declaration&>(*this);
        vec.empend(const_cast<Declaration&>(decl));
        return vec;
    }
    return {};
}

NonnullRefPtrVector<Declaration> ForStatement::declarations() const
{
    NonnullRefPtrVector<Declaration> declarations;
    if (m_init)
        declarations.append(m_init->declarations());
    if (m_body)
        declarations.append(m_body->declarations());
    return declarations;
}

NonnullRefPtrVector<Declaration> BlockStatement::declarations() const
{
    NonnullRefPtrVector<Declaration> declarations;
    for (auto& statement : m_statements) {
        declarations.append(statement.declarations());
    }
    return declarations;
}

void IfStatement::dump(FILE* output, size_t indent) const
{
    ASTNode::dump(output, indent);
    if (m_predicate) {
        print_indent(output, indent + 1);
        outln(output, "Predicate:");
        m_predicate->dump(output, indent + 1);
    }
    if (m_then) {
        print_indent(output, indent + 1);
        outln(output, "Then:");
        m_then->dump(output, indent + 1);
    }
    if (m_else) {
        print_indent(output, indent + 1);
        outln(output, "Else:");
        m_else->dump(output, indent + 1);
    }
}

NonnullRefPtrVector<Declaration> IfStatement::declarations() const
{
    NonnullRefPtrVector<Declaration> declarations;
    if (m_predicate)
        declarations.append(m_predicate->declarations());
    if (m_then)
        declarations.append(m_then->declarations());
    if (m_else)
        declarations.append(m_else->declarations());
    return declarations;
}

void NamespaceDeclaration::dump(FILE* output, size_t indent) const
{
    ASTNode::dump(output, indent);
    print_indent(output, indent + 1);
    outln(output, "{}", m_name);
    for (auto& decl : m_declarations)
        decl.dump(output, indent + 1);
}

void NullPointerLiteral::dump(FILE* output, size_t indent) const
{
    ASTNode::dump(output, indent);
}

void Name::dump(FILE* output, size_t indent) const
{
    ASTNode::dump(output, indent);
    print_indent(output, indent);
    outln(output, "{}", full_name());
}

String Name::full_name() const
{
    StringBuilder builder;
    if (!m_scope.is_empty()) {
        for (auto& scope : m_scope) {
            builder.appendff("{}::", scope.m_name);
        }
    }
    return String::formatted("{}{}", builder.to_string(), m_name.is_null() ? "" : m_name->m_name);
}

String TemplatizedName::full_name() const
{
    StringBuilder name;
    name.append(Name::full_name());
    name.append('<');
    for (auto& type : m_template_arguments) {
        name.append(type.to_string());
    }
    name.append('>');
    return name.to_string();
}

void CppCastExpression::dump(FILE* output, size_t indent) const
{
    ASTNode::dump(output, indent);

    print_indent(output, indent);
    outln(output, "{}", m_cast_type);

    print_indent(output, indent + 1);
    outln(output, "<");
    if (m_type)
        m_type->dump(output, indent + 1);
    print_indent(output, indent + 1);
    outln(output, ">");

    if (m_expression)
        m_expression->dump(output, indent + 1);
}

void SizeofExpression::dump(FILE* output, size_t indent) const
{
    ASTNode::dump(output, indent);
    if (m_type)
        m_type->dump(output, indent + 1);
}

void BracedInitList::dump(FILE* output, size_t indent) const
{
    ASTNode::dump(output, indent);
    for (auto& exp : m_expressions) {
        exp.dump(output, indent + 1);
    }
}

void CStyleCastExpression::dump(FILE* output, size_t indent) const
{
    ASTNode::dump(output, indent);
    if (m_type)
        m_type->dump(output, indent + 1);
    if (m_expression)
        m_expression->dump(output, indent + 1);
}

void Constructor::dump(FILE* output, size_t indent) const
{
    print_indent(output, indent);
    outln(output, "C'tor");
    print_indent(output, indent + 1);
    outln(output, "(");
    for (const auto& arg : m_parameters) {
        arg.dump(output, indent + 1);
    }
    print_indent(output, indent + 1);
    outln(output, ")");
    if (!m_definition.is_null()) {
        m_definition->dump(output, indent + 1);
    }
}

void Destructor::dump(FILE* output, size_t indent) const
{
    print_indent(output, indent);
    outln(output, "D'tor");
    print_indent(output, indent + 1);
    outln(output, "(");
    for (const auto& arg : m_parameters) {
        arg.dump(output, indent + 1);
    }
    print_indent(output, indent + 1);
    outln(output, ")");
    if (!m_definition.is_null()) {
        m_definition->dump(output, indent + 1);
    }
}

}
