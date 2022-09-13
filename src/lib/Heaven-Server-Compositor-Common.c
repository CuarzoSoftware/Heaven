#include "Heaven-Server-Compositor-Common.h"

struct hv_compositor_struct
{
    void *user_data;
};

void hv_compositor_ser_user_data(hv_compositor *compositor, void *user_data)
{
    compositor->user_data = user_data;
}

void *hv_compositor_get_user_data(hv_compositor *compositor)
{
    return compositor->user_data;
}
