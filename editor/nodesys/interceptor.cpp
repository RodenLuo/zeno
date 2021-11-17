#include "interceptor.h"
#include <zeno/dop/Node.h>


void Interceptor::toDopGraph
    ( QDMGraphicsScene *scene
    , dop::SceneGraph *d_scene
    )
{
    std::map<QDMGraphicsSocketIn *, QDMGraphicsSocketOut *> links;
    for (auto const &link: scene->links) {
        links.emplace(link->dstSocket, link->srcSocket);
    }

    std::map<QDMGraphicsNode *, dop::Node *> nodes;
    for (auto const &node: scene->nodes) {
        auto d_node = std::make_unique<dop::Node>();

        d_node->desc = node->getDescriptor();
        d_node->name = node->getName();
        d_node->inputs.resize(node->socketIns.size());
        d_node->outputs.resize(node->socketOuts.size());

        nodes.emplace(node.get(), d_node.get());
        d_scene->nodes.insert(std::move(d_node));
    }

    for (auto const &node: scene->nodes) {
        for (size_t i = 0; i < node->socketIns.size(); i++) {
            dop::Input input{};
            auto sockIn = node->socketIns[i].get();
            input.value = sockIn->value;

            if (auto it = links.find(sockIn); it != links.end()) {
                auto *sockOut = it->second;
                auto srcNode = static_cast<QDMGraphicsNode *>(sockOut->parentItem());
                input.sockid = srcNode->socketOutIndex(sockOut);
                input.node = nodes.at(srcNode);
            }

            auto d_node = nodes.at(node.get());
            d_node->inputs[i] = input;
        }
    }
}
