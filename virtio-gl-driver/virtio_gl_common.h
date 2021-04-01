#define VIRTIO_ID_GL 69


enum
{
    VIRTGL_CMD_WRITE = 100,
};

enum
{
    VIRTGL_OPENGL_CREATE_WINDOW = 200,
};

typedef struct VirtioGLArg   VirtioGLArg;

// function args
struct VirtioGLArg
{
	int32_t cmd;
	uint64_t rnd;
	uint64_t para;
	
	uint64_t pA;
	uint32_t pASize;

	uint64_t pB;
	uint32_t pBSize;

	uint32_t flag;

};