#include <CZ/Heaven/Client/HNObject.h>
#include <CZ/Heaven/Client/HNClient.h>

using namespace CZ;

HNClientAPI::HNObject::~HNObject() noexcept
{
    m_client->removeObject(this);
}

CZ::HNClientAPI::HNObject::HNObject(std::shared_ptr<HNClient> client, UInt32 id, Type type) noexcept :
    m_client(client), m_id(id), m_type(type)
{
    client->addObject(this);
    client->sendCreateObject(this);
}
