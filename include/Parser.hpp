#pragma once
#include <Logger.hpp>
#include <Server.hpp>
#include <Token.hpp>
#include <Webserver.hpp>
#include <array>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#define PARSER_FUNC_N 2
#define BLOCK_FUNC_N 8

/* Circle inclusion, so doesn't compile without this parser definition */
class Parser;
/* Parser member function typedef */
typedef int (Parser::*ParserFunc)(void *dest,
                                  std::vector<std::string> dataLine);

/* Key - parser function pair */
typedef struct s_parseFuncPair {
  std::string key;
  ParserFunc func;
} t_parseFuncPair;

typedef std::vector<std::string> t_dataLine;

/*
        Example datablock:
        server {
                listen: 80;
        }
*/
struct DataBlock {
  // Name of datablock (positioned before the open_curl)
  std::vector<std::string> name;
  // Nested blocks with brackets
  std::vector<DataBlock> blocks;
  // Semi-colon ended lines in config file
  std::vector<t_dataLine> dataLines;
};

struct AbstractSyntaxTree {
  // Server blocks opened and closed with curls
  std::vector<DataBlock> blocks;
  // Random lines with config info
  std::vector<t_dataLine> dataLines;
};

/**
 * Takes a vector of tokens and parses them into a vector of servers
 */
class Parser {
 public:
  Parser(std::vector<Token> &tokens);
  ~Parser();
  int parseTokens(std::vector<Server> *pServers);

 private:
  Token::ETokenType lookAhead(std::vector<Token>::iterator it,
                              std::vector<Token>::iterator const &end);
  std::vector<std::string> parseUntilSpecialChar();
  int parseDataBlock(std::vector<DataBlock> *pDest);
  int makeAst();

  int parseAst(std::vector<Server> *pServers);
  int parseServerDataLines(Server *pServer,
                           std::vector<t_dataLine> const &lines);
  int parseDataBlocks(Server *pServer, std::vector<DataBlock> const &blocks);
  int parseLocationBlocks(Route *pRoute, std::vector<t_dataLine> const &lines);
  int convertBlockToServer(Server *pServer, DataBlock block);

  /* AST line parsing functions */
  int parsePort(void *dest, t_dataLine line);
  int parseMaxBodySize(void *dest, t_dataLine line);
  int parseServerName(void *dest, t_dataLine line);
  /* AST block parsing functions */
  int parseErrorPage(void *dest, t_dataLine line);
  int parseRoot(void *dest, t_dataLine line);
  int parseAutoIndex(void *dest, t_dataLine line);
  int parseIndex(void *dest, t_dataLine line);
  int parseCgi(void *dest, t_dataLine line);
  int parseCgiParam(void *dest, t_dataLine line);
  int parseMethods(void *dest, t_dataLine line);
  /* table with key value pairs for parsing AST */
  static t_parseFuncPair lineParsingFuncs[PARSER_FUNC_N];
  static t_parseFuncPair blockParsingFuncs[BLOCK_FUNC_N];

  std::vector<Token> _tokens;
  AbstractSyntaxTree _tree;
  std::vector<Token>::iterator _it;
};
