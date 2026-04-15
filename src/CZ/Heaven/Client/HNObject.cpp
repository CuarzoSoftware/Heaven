#include <CZ/Heaven/Client/HNObject.h>

using namespace CZ;

CZ::Client::HNObject::HNObject(std::shared_ptr<HNClient> client, UInt32 id, Type type) noexcept :
    m_client(client), m_id(id), m_type(type)
{

}
