#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include <clang/Tooling/CommonOptionsParser.h>
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Frontend/FrontendAction.h"
#include "clang/Tooling/Tooling.h"

using namespace clang;
using namespace clang::ast_matchers;
using namespace std;
using namespace clang::driver;
using namespace clang::tooling;
using namespace llvm;
#include <vector>


/*                             
                            Matcher definition
    Capture an entire statement that has a function declaration as an ancestor.
    Any nodes 'matching' the description above will be referred as "function"
*/
StatementMatcher Function = compoundStmt(hasAncestor(functionDecl().bind("function")));

/*
    Useful class to run a 'matcher'.
    The matchers are run from the a ClangTool, paired with a 'MatchCallback' and registered
    with a 'MatchFinder' object.
*/
class FunctionFinder : public MatchFinder::MatchCallback {
public :
  
  // Vector to store all the function names
  std::vector<string> func_name;

  virtual void run(const MatchFinder::MatchResult &Result) {
    ASTContext *Context = Result.Context;

    if (const clang::FunctionDecl *FD = Result.Nodes.getNodeAs<clang::FunctionDecl>("function")){
      // Only match AST nodes belonging to the source file
      if (Context->getSourceManager().isWrittenInMainFile(FD->getLocation()))
        {
          // Store the function name
          func_name.push_back(FD->getNameAsString());

        }
    } 
}
};

class LoopFinder : public MatchFinder::MatchCallback {
private:
  int n_loop = 0;
public :
  virtual void run(const MatchFinder::MatchResult &Result) {
    ASTContext *Context = Result.Context;

    if (const ForStmt *FS = Result.Nodes.getNodeAs<clang::ForStmt>("forLoop")){
      if (!FS || !Context->getSourceManager().isWrittenInMainFile(FS->getForLoc()))
        return;
      n_loop += 1;
      // Debug print.
      // llvm::outs() << "for loop "<<n_loop<<"\n";
    }
    if (const WhileStmt *WS = Result.Nodes.getNodeAs<clang::WhileStmt>("whileLoop")){
      if (!WS || !Context->getSourceManager().isWrittenInMainFile(WS->getWhileLoc()))
        return;
      // Debug print.
      // llvm::outs() << "while loop "<<n_loop<<"\n";
      n_loop += 1;
    }

    if (const DoStmt *DS = Result.Nodes.getNodeAs<clang::DoStmt>("doLoop")){
      if (!DS || !Context->getSourceManager().isWrittenInMainFile(DS->getDoLoc()))
        return;
      // Debug print.
      // llvm::outs() << "do loop "<<n_loop<<"\n";
      n_loop += 1;
    }
    
  }
    void reset_number_of_loop() {
      n_loop = 0;
    }

    int get_number_of_loop() {
      return n_loop;
    }
};


class Function_Call_Finder : public MatchFinder::MatchCallback {
public :
  
  int n_call = 0;
  std::vector<string> declared_func_names;
  std::vector<string> func_called;

  // Get a list of all the function created in the source file
  // Filter out any function that is not from this list like the 'operator<<' function
  void set_func_name(std::vector<string> func){
    declared_func_names = func;
  }

  void print_func_name(){
    for (auto f: declared_func_names){
      llvm::outs() << "this is func " << f << "\n";
  }
  };


  virtual void run(const MatchFinder::MatchResult &Result) {
    ASTContext *Context = Result.Context;

    if (const clang::CallExpr *CE = Result.Nodes.getNodeAs<clang::CallExpr>("function_call")){
      
      // Filter out any function that is not from this list like the 'operator<<' function
      if (std::find(declared_func_names.begin(), declared_func_names.end(), CE->getDirectCallee()->getNameAsString()) != declared_func_names.end()){
        //llvm::outs() << "name is " << CE->getDirectCallee()->getNameAsString() << "\n";
        //getNameAsString()
        func_called.push_back(CE->getDirectCallee()->getNameAsString());
        CE->getDirectCallee()->getNumParams(); 
        llvm::outs() << "'" << CE->getDirectCallee()->getNameAsString()<< "' has " << CE->getDirectCallee()->getNumParams() << " parameter(s)\n";
        //n_call += 1;
      }
    } 
  }
};


static cl::OptionCategory MyToolCategory("Loop convert options");

int main(int argc, const char **argv) {

  llvm::Expected<clang::tooling::CommonOptionsParser> op = CommonOptionsParser::create(argc, argv, MyToolCategory); //new

  if (!op) {
    // Fail gracefully for unsupported options.
    llvm::errs() << op.takeError();
    return 1;
  }

  // create a new LibTooling instance                                                         
  ClangTool Tool(op->getCompilations(), op->getSourcePathList());


  FunctionFinder FunctionFinder_;
  MatchFinder Finder;

  Finder.addMatcher(Function, &FunctionFinder_);
  
  // Nothing is return. The result of operation is store within the
  // 'FunctionFinder_' class.
  int result = Tool.run(newFrontendActionFactory(&Finder).get());

  std::vector<string> func_name = FunctionFinder_.func_name;

  // FIXME: Figure out why there functions are called multiple time
  // causing duplicates to be stored.
  
  // remove duplicates
  func_name.erase(std::unique(func_name.begin(), func_name.end()), func_name.end());

  /*
    Now that we have all the function names, we can check all the loops
    which have ancestor 'func_name'.
  */

  int number_of_loops = 0;
  int number_of_calls = 0;
  for (auto f: func_name){
    MatchFinder Finder;

    // Matching all loops
    std::vector<StatementMatcher> Loop = {
      forStmt(hasAncestor(functionDecl(hasName(f)))).bind("forLoop"),
      whileStmt(hasAncestor(functionDecl(hasName(f)))).bind("whileLoop"),
      doStmt(hasAncestor(functionDecl(hasName(f)))).bind("doLoop")};

    for (StatementMatcher loop : Loop){
      LoopFinder LoopFinder_;
      Finder.addMatcher(loop, &LoopFinder_);
      result = Tool.run(newFrontendActionFactory(&Finder).get());

      number_of_loops = LoopFinder_.get_number_of_loop();
    }

  llvm::outs() <<"'" << f << "' has " << number_of_loops << " loop(s) \n";

  }


  std::vector<string> func_called;
  for (auto f: func_name){
    MatchFinder Finder_call;
    //StatementMatcher Function_Call = compoundStmt(callExpr().bind("function_call"), hasAncestor(functionDecl(hasName(f))));
    StatementMatcher Function_Call = callExpr(hasAncestor(functionDecl(hasName(f)))).bind("function_call");
    Function_Call_Finder Function_Call_Finder_;
    Function_Call_Finder_.set_func_name(func_name);
    Finder_call.addMatcher(Function_Call, &Function_Call_Finder_);
    result = Tool.run(newFrontendActionFactory(&Finder_call).get());

    func_called.insert(func_called.end(), Function_Call_Finder_.func_called.begin(), Function_Call_Finder_.func_called.end());
  }

std::map<std::string , int> histogram;

for (const string & s : func_called) { ++histogram[s]; }

for (const auto & p : histogram)
{
  // Only functions that are called at least once will be printed
  llvm::outs() <<"'"<< p.first << "' is called " << p.second << " time(s).\n";
}


  // potential useful functions:
  // 'matchesName', hasAnyParameter, parmVarDecl, decl
  // callExpr
  // get number of arguments of a function call
  //    https://clang.llvm.org/doxygen/classclang_1_1CallExpr.html
  return result;
}