/*
 * QEMU educational PCI device
 *
 * Copyright (c) 2012-2015 Jiri Slaby
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "qemu/osdep.h"
#include "qemu/units.h"
#include "hw/pci/pci.h"
#include "hw/hw.h"
#include "hw/pci/msi.h"
#include "qemu/timer.h"
#include "qom/object.h"
#include "qemu/main-loop.h" /* iothread mutex */
#include "qemu/module.h"
#include "qapi/visitor.h"
#include "hw/virtio/virtio-pci.h"

#include "trace.h"

#include "../../../virtio-gl-driver/virtio_gl_common.h"

#include <fcntl.h>

#include <stdio.h>

#if 1
#define printf(fmt, arg...) {\
    FILE *fptr = fopen("log.txt", "a+");\
	fprintf(fptr, fmt, ##arg);\
    fclose(fptr);\
}
#else

#endif

static void* gpa_to_hva(uint64_t pa) 
{
	MemoryRegionSection section;

	section = memory_region_find(get_system_memory(), (ram_addr_t)pa, 1);
	if ( !int128_nz(section.size) || !memory_region_is_ram(section.mr)){
		error("addr %p in rom\n", (void*)pa); 
		return 0;
	}

	return (memory_region_get_ram_ptr(section.mr) + section.offset_within_region);
}

//////////////////////////////////////////////////////////////////////////////
// begin of VIRTIO GL DEVICE info
#define TYPE_VIRTIO_GL "virtio-gl-device"
typedef struct VirtIOGL VirtIOGL;
typedef struct VirtIOGLConf VirtIOGLConf;
#define VIRTIO_GL(obj) OBJECT_CHECK(VirtIOGL, (obj), TYPE_VIRTIO_GL)

struct VirtIOGLConf
{
    uint64_t mem_size;
};

struct VirtIOGL
{
    VirtIODevice parent_obj;
    VirtQueue *vq;
    VirtIOGLConf conf;
};

static int vgl_cmd_write(VirtioGLArg *arg)
{
    printf("pA = %d , pASize = %u, pB = %d , pBSize = %u\n", arg->pA, arg->pASize, arg->pB, arg->pBSize);

    int *intPointer;
	char *charPointer;

    intPointer  = gpa_to_hva(arg->pA);
	charPointer = gpa_to_hva(arg->pB);

    printf("A address= %llu ,B address= '%llu'\n", intPointer, charPointer);

    uint32_t sizeA, sizeB;
    sizeA = arg->pASize;
    sizeB = arg->pBSize;

    int *testInt;
    char *testChar;
    testInt = (int*)malloc(4);
    testChar = (char*)malloc(sizeB);

	memcpy(testInt, intPointer, 4);
    memcpy(testChar, charPointer, sizeB);
	
    printf("int= %d ,char= '%s'\n", *testInt, testChar);
    return 0;
}

static int vgl_create_window(VirtioGLArg *arg)
{
    printf("create window \n");
    return 0;
}

//####################################################################
//   class basic callback functions
//####################################################################
static void virtio_gl_cmd_handle(VirtIODevice *vdev, VirtQueue *vq)
{
    VirtQueueElement *elem;
    VirtioGLArg *arg;
    size_t s;

    arg = malloc(sizeof(VirtioGLArg));
    elem = virtqueue_pop(vq, sizeof(VirtQueueElement));
    // if(elem){
    //     s = iov_to_buf(elem->in_sg, elem->in_num, 0,
    //                    arg, sizeof(VirtioGLArg));
    //     printf("arg get size: %d \n", s);
    //     printf("-----cmd: %d \n", arg->cmd);
    // }
    // else{
    //     printf("no arg\n");
    // }
    while(elem){
        s = iov_to_buf(elem->in_sg, elem->in_num, 0,
                       arg, sizeof(VirtioGLArg));
        printf("arg get size: %d \n", s);
        printf("-----cmd: %d \n", arg->cmd);

        switch (arg->cmd)
        {
        case VIRTGL_CMD_WRITE:
            vgl_cmd_write(arg);
            break;
        case VIRTGL_OPENGL_CREATE_WINDOW:
            vgl_create_window(arg);
            break;
        default:
            break;
        }
        
        elem = virtqueue_pop(vq, sizeof(VirtQueueElement));
    }
    // while (virtqueue_pop(vq, &elem))
    // {
    //     printf("get from virtio queue\n");
    //     printf("cmd: %d \n", arg->cmd);
    //     switch (arg->cmd)
    //     {
    //     case VIRTGL_CMD_WRITE:
    //         vgl_cmd_write(arg);
    //         break;
    //     case VIRTGL_OPENGL_CREATE_WINDOW:
    //         vgl_create_window(arg);
    //         break;
    //     default:
    //         break;
    //     }
    // }
    // free(arg);
}

static uint64_t virtio_gl_get_features(VirtIODevice *vdev, uint64_t features, Error **errp)
{
    return features;
}

static void virtio_gl_device_realize(DeviceState *dev, Error **errp)
{
    VirtIODevice *vdev = VIRTIO_DEVICE(dev);
    VirtIOGL *gl = VIRTIO_GL(dev);

    virtio_init(vdev, "virtio-gl", VIRTIO_ID_GL, sizeof(VirtIOGLConf));

    gl->vq = virtio_add_queue(vdev, 1024, virtio_gl_cmd_handle);
}

/*
   get the configure
ex: -device virtio-gl-device,size=2G,.....
DEFINE_PROP_SIZE(config name, device struce, element, default value)
 */
static Property virtio_gl_properties[] =
    {
        DEFINE_PROP_SIZE("size", VirtIOGL, conf.mem_size, 0),
        DEFINE_PROP_END_OF_LIST(),
};

static void virtio_gl_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    VirtioDeviceClass *vdc = VIRTIO_DEVICE_CLASS(klass);

    trace_virtio_gl_device_class_init(dc);

    device_class_set_props(vdc, virtio_gl_properties);

    set_bit(DEVICE_CATEGORY_MISC, dc->categories);

    vdc->get_features = virtio_gl_get_features;
    vdc->realize = virtio_gl_device_realize;
}

static void virtio_gl_instance_init(Object *obj)
{
}

static const TypeInfo virtio_gl_device_info = {
    .name = TYPE_VIRTIO_GL,
    .parent = TYPE_VIRTIO_DEVICE,
    .instance_size = sizeof(VirtIOGL),
    .instance_init = virtio_gl_instance_init,
    .class_init = virtio_gl_class_init,
};
// end of VIRTIO GL DEVICE info
//////////////////////////////////////////////////////////////////////////////

static void virtio_gl_device_register_types(void)
{
    type_register_static(&virtio_gl_device_info);
}
type_init(virtio_gl_device_register_types)

//////////////////////////////////////////////////////////////////////////////
// begin of VIRTIO GL PCI info
#define TYPE_VIRTIO_GL_PCI "virtio-gl-pci"
#define VIRTIO_GL_PCI(obj) OBJECT_CHECK(VirtIOGLPCI, (obj), TYPE_VIRTIO_GL_PCI)
    typedef struct VirtIOGLPCI VirtIOGLPCI;

struct VirtIOGLPCI
{
    VirtIOPCIProxy parent_obj;
    VirtIOGL vdev;
};

static void virtio_gl_pci_realize(VirtIOPCIProxy *vpci_dev, Error **errp)
{
    VirtIOGLPCI *gl = VIRTIO_GL_PCI(vpci_dev);
    DeviceState *vdev = DEVICE(&gl->vdev);

    virtio_pci_force_virtio_1(vpci_dev);
    trace_virtio_gl_dev_realize(vdev);
    if (!qdev_realize(vdev, BUS(&vpci_dev->bus), errp))
    {
        trace_virtio_gl_dev_realize(vdev);
        return;
    }
}

static Property virtio_gl_pci_properties[] = {
    DEFINE_PROP_BIT("ioeventfd", VirtIOPCIProxy, flags,
                    VIRTIO_PCI_FLAG_USE_IOEVENTFD_BIT, true),
    DEFINE_PROP_UINT32("vectors", VirtIOPCIProxy, nvectors, 2),
    DEFINE_PROP_END_OF_LIST(),
};

static void virtio_gl_pci_class_init(ObjectClass *klass, void *data)
{
    DeviceClass *dc = DEVICE_CLASS(klass);
    PCIDeviceClass *pcidev_k = PCI_DEVICE_CLASS(klass);
    VirtioPCIClass *k = VIRTIO_PCI_CLASS(klass);

    trace_virtio_gl_pci_class_init(dc);

    set_bit(DEVICE_CATEGORY_MISC, dc->categories);
    device_class_set_props(dc, virtio_gl_pci_properties);

    k->realize = virtio_gl_pci_realize;
    pcidev_k->vendor_id = PCI_VENDOR_ID_REDHAT_QUMRANET;
    pcidev_k->device_id = PCI_DEVICE_ID_VIRTIO_GL;
    pcidev_k->revision = VIRTIO_PCI_ABI_VERSION;
    pcidev_k->class_id = PCI_CLASS_OTHERS;
}

static void virtio_gl_pci_instance_init(Object *obj)
{
    VirtIOGLPCI *dev = VIRTIO_GL_PCI(obj);

    virtio_instance_init_common(obj, &dev->vdev, sizeof(dev->vdev),
                                TYPE_VIRTIO_GL);
}

static const VirtioPCIDeviceTypeInfo virtio_gl_pci_info = {
    .base_name = "virtio-gl-pci-base",
    .parent = TYPE_VIRTIO_PCI,
    .generic_name = TYPE_VIRTIO_GL_PCI,
    .instance_size = sizeof(VirtIOGLPCI),
    .instance_init = virtio_gl_pci_instance_init,
    .class_init = virtio_gl_pci_class_init,
};
// end of VIRTIO GL PCI info
//////////////////////////////////////////////////////////////////////////////

static void virtio_gl_pci_register_types(void)
{
    virtio_pci_types_register(&virtio_gl_pci_info);
}
type_init(virtio_gl_pci_register_types)
