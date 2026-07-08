#include "../base_parser.hpp"
#include "../report/report.hpp"
#include <filesystem>
#include <variant>

namespace assembly {

struct TokenCoordinate {
   std::size_t row, col;
};

enum class TokenType {
   Unknown,
   // [0-9].*
   Number,
   // [A-z]|[0-9].*
   Label,
   Exclamation,
   Minus,
   Plus,
   Pipe,
   Ampersand,
   AtSymbol,
   Semicolon,
   OpenParen,
   CloseParen,
   Equal,
   Newline,
};

struct Token {
   TokenType type;
   TokenCoordinate start_coord, end_coord;
   std::variant<int, std::size_t, std::string> value;
};
enum class Operator {
   None,

   // unary
   Neg,
   Not,

   // binary
   Add,
   Sub,
   And,
   Or,
};

enum class Address {
   None,
   A,
   D,
   M,
};

// operand can either be in the interval 0, 1 or an address name
using Operand = std::variant<Address, std::size_t>;

struct UnaryComp {
   TokenCoordinate start;
   TokenCoordinate end;
   Operator op;
   Operand operand;
};

struct BinaryComp {
   TokenCoordinate start;
   TokenCoordinate end;
   Address left;
   Operator op;
   Operand right;
};

enum class Jump {
   None,

   JGT,
   JEQ,
   JLT,
   JGE,
   JNE,
   JLE,
   JMP,
};

enum class Destination {
   None,

   A,
   D,
   M,
   MD,
   AM,
   AD,
   AMD
};

struct AInstr {
   TokenCoordinate start_coord;
   TokenCoordinate end_coord;
   std::variant<std::size_t, std::string> value;
};

struct CInstr {
   TokenCoordinate start;
   TokenCoordinate end;
   Destination dest;
   std::variant<UnaryComp, BinaryComp> comp;
   Jump jump;
};

struct Label {
   TokenCoordinate start_coord;
   TokenCoordinate end_coord;
   std::string value;
};

using Instruction = std::variant<Label, AInstr, CInstr>;

class Parser final : public BaseParser<Token, TokenType> {
   std::vector<Instruction> m_instructions;

   std::optional<Jump> parse_jump();
   std::optional<Destination> parse_dest();
   std::optional<std::variant<UnaryComp, BinaryComp>> parse_comp();

   std::optional<CInstr> parse_cinstr();
   std::optional<AInstr> parse_ainstr();
   std::optional<Label> parse_label();

   public:
   using BaseParser::BaseParser;
   std::optional<std::vector<Instruction>> parse();
};

class Lexer final {
   std::ifstream m_asm_file;
   std::vector<Token> m_tokens { };
   std::size_t m_curr_x { 0 }, m_curr_y { 0 };

   Token lex_number();
   Token lex_label();
   void ignore_comment();

   public:
   explicit Lexer(const std::filesystem::path filepath)
       : m_asm_file { filepath } { }
   std::vector<Token> tokenize();
};

// operand can either be in the interval 0, 1 or an address name
using Operand = std::variant<Address, std::size_t>;

class CodeGen {
   std::vector<Instruction> m_instructions;
   std::uint16_t m_pc { 0 };
   std::string m_error_report { "" };
   report::Context m_reporter;

   std::uint16_t compile_cinstr_dest(CInstr ctx) const noexcept;
   std::optional<std::uint16_t> compile_cinstr_comp(CInstr ctx);
   std::uint16_t compile_cinstr_jump(CInstr ctx) const noexcept;
   inline void emit_error(TokenCoordinate start, TokenCoordinate end, std::string_view error_msg);

   public:
   explicit CodeGen(std::vector<Instruction> instructions, std::filesystem::path filepath);
   std::optional<std::vector<std::uint16_t>> compile();
   std::string get_error_report();
};

std::string to_string(std::vector<std::uint16_t> asm_instructions);

std::optional<std::string> disassemble(std::uint16_t instruction);
std::optional<std::string> disassemble(std::string_view instructions);
}
