#include "Heaven-Server-Compositor-Common.h"

struct hn_compositor_struct
{
    void *user_data;
};

void hn_compositor_ser_user_data(hn_compositor *compositor, void *user_data)
{
    compositor->user_data = user_data;
}

void *hn_compositor_get_user_data(hn_compositor *compositor)
{
    return compositor->user_data;
}
