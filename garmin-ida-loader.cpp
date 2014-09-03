#include <windows.h>
#include <vector>
#include <..\ldr\idaldr.h>

using namespace std;
typedef vector<unsigned char> CHARS;

const char FW_SEGMENT[] = "GRMFW";

struct FWINFO
{
	unsigned base;
	unsigned short hwid;
	unsigned short swvr;
};

unsigned char* memsrch_r(unsigned char* buf, unsigned cbuf, unsigned char* what, unsigned cwhat)
{
	if(cbuf < cwhat) return 0;
	unsigned char* p = buf + (cbuf - cwhat);
	for(; p >= buf; p--)
	{
		if(0==memcmp(p, what, cwhat)) return p;
	}
	return 0;
}

bool GetFirmwareInfo(const unsigned char* p, unsigned size, FWINFO& fwi)
{
	if(size < 0x100) return false;

	static const unsigned char FWEND[] = {0xFF, 0xFF, 0x5A, 0xA5};
	unsigned char* pend = memsrch_r((unsigned char*)p, size, (unsigned char*)FWEND, sizeof(FWEND));
	if(!pend) return false;
	pend += 2;

	unsigned* dw = (unsigned*)p;

	switch(dw[0])
	{
	case 0xE59FF008:
		fwi.base = dw[4] - 0x14;
		break;
	case 0xE59FF00C:
		fwi.base = dw[5] - 0x18;
		break;
	case 0xEA000002:
	case 0xEA000003:
	default:
		fwi.base = dw[1] - (pend - p);
	}

	if((fwi.base % 4) != 0) return false; //must be aligned
	if(fwi.base + size < fwi.base) return false; //overflow
	if((dw[2] % 2) != 0 || dw[2] - fwi.base >= pend - p - 3)  return false; //align & bounds
	if((dw[3] % 2) != 0 || dw[3] - fwi.base >= pend - p - 3)  return false;
	fwi.hwid = *(unsigned short*)(p + dw[2] - fwi.base);
	fwi.swvr = *(unsigned short*)(p + dw[3] - fwi.base);
	return true;
}

bool LoadFile(linput_t * file, CHARS & data)
{
	unsigned size = qlsize(file);
	data.resize(size);

	if(size > 0)
	{
		if(qlread(file, &data[0], size)==-1)
		{
			data.resize(0);
		}
	}
	return data.size()==size;
}

int  idaapi accept_file(linput_t * file, char fileformatname[MAX_FILE_FORMAT_NAME], int n)
{
	if(n>0) return 0;

	FWINFO fwi;
	CHARS data;
	if(!LoadFile(file, data) || data.size()==0 || !GetFirmwareInfo(&data[0], data.size(), fwi)) 
	{ 
		return 0;
	}

	qsnprintf(fileformatname, MAX_FILE_FORMAT_NAME, "Garmin Firmware");
	
	set_processor_type("ARM", SETPROC_ALL);

	return 1 | ACCEPT_FIRST;
}

void idaapi load_file  (linput_t * file, ushort neflags, const char * formatname)
{
	FWINFO fwi;
	CHARS data;
	if(!LoadFile(file, data) || data.size()==0) 
	{
		loader_failure("cannot read input file\n");
	}

	if(!GetFirmwareInfo(&data[0], data.size(), fwi))
	{
		loader_failure("input file has unknown format\n");
	}

	file2base(file, 0, fwi.base, fwi.base + data.size(), true);
	   
	if(!add_segm(0, fwi.base, fwi.base + data.size(), FW_SEGMENT, CLASS_CODE)) 
	{
		loader_failure("cannot create code segment\n");
	}

	segment_t *s = get_segm_by_name(FW_SEGMENT);
	set_segm_addressing(s, 1);
	add_entry(fwi.base, fwi.base, "fw_base", true);
	return;
}

int  idaapi save_file  (FILE * file, const char * formatname)
{
	if (file == NULL) return 1;

	segment_t *s = get_segm_by_name(FW_SEGMENT);
	if (!s) return 0;

	base2file(file, 0, s->startEA, s->endEA);
	return 1;
}

__declspec(dllexport) 
loader_t LDSC = 
{
	IDP_INTERFACE_VERSION,
	0,
	accept_file,
	load_file,
	save_file,
	NULL,
	NULL,
};