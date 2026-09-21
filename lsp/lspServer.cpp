#include "lspServer.h"

#include <iostream>

#include "langsvr/content_stream.h"
#include "langsvr/reader.h"
#include "langsvr/writer.h"

namespace ionsl
{
    class StdinReader : public langsvr::Reader
    {
    public:
        size_t Read(std::byte* out, size_t count) override
        {
            std::cin.read(
                reinterpret_cast<char*>(out),
                static_cast<std::streamsize>(count));

            return static_cast<size_t>(std::cin.gcount());
        }
    };

    class StdoutWriter : public langsvr::Writer
    {
    public:
        langsvr::Result<langsvr::SuccessType> Write(
            const std::byte* in,
            size_t count) override
        {
            std::cout.write(
                reinterpret_cast<const char*>(in),
                static_cast<std::streamsize>(count));

            if (!std::cout)
            {
                return langsvr::Failure{"failed to write to stdout"};
            }

            std::cout.flush();

            return langsvr::Success;
        }
    };


    LspServer::LspServer()
    {
        m_session.Register(
            [this](const langsvr::lsp::InitializeRequest& request)
            {
                return handleInitialize(request);
            });

        m_session.Register(
            [this](const langsvr::lsp::TextDocumentDidOpenNotification& notification)
            {
                return handleDidOpen(notification);
            });

        m_session.Register(
            [this](const langsvr::lsp::TextDocumentDidChangeNotification& notification)
            {
                return handleDidChange(notification);
            });
    }

    langsvr::Result<langsvr::lsp::InitializeResult, langsvr::lsp::InitializeError> LspServer::handleInitialize(const langsvr::lsp::InitializeRequest &request)
    {
        langsvr::lsp::InitializeResult result;

        result.capabilities.text_document_sync =langsvr::lsp::TextDocumentSyncKind::kFull;

        return result;
    }

    langsvr::Result<langsvr::SuccessType> LspServer::handleDidOpen(const langsvr::lsp::DidOpenTextDocumentParams &notification)
    {
        return langsvr::Success;
    }

    langsvr::Result<langsvr::SuccessType> LspServer::handleDidChange(const langsvr::lsp::DidChangeTextDocumentParams &notification)
    {
        return langsvr::Success;
    }

    int LspServer::run()
    {
        StdinReader reader;
        StdoutWriter writer;

        m_session.SetSender(
            [&writer](std::string_view message)
            {
                return langsvr::WriteContent(writer, message);
            });

        while (true)
        {
            auto message = langsvr::ReadContent(reader);

            if (message != langsvr::Success)
            {
                std::cerr << "LSP read error: "
                          << message.Failure().reason
                          << '\n';

                return 1;
            }

            auto result = m_session.Receive(message.Get());

            if (result != langsvr::Success)
            {
                std::cerr << "LSP error: "
                          << result.Failure().reason
                          << '\n';

                return 1;
            }
        }
    }
}
