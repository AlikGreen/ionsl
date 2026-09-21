#pragma once

#include "langsvr/session.h"
#include "langsvr/lsp/lsp.h"

namespace ionsl
{
class LspServer
{
public:
    LspServer();

    langsvr::Result<langsvr::lsp::InitializeResult, langsvr::lsp::InitializeError> handleInitialize(const langsvr::lsp::InitializeRequest& request);

    langsvr::Result<langsvr::SuccessType> handleDidOpen(const langsvr::lsp::DidOpenTextDocumentParams &notification);
    langsvr::Result<langsvr::SuccessType> handleDidChange(const langsvr::lsp::DidChangeTextDocumentParams& notification);

    int run();
private:
    langsvr::Session m_session;
};
}
