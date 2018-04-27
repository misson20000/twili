#ifdef __cplusplus
extern "C" {
#endif

#define	PSF_MODE_512		1
#define	PSF_MODE_HAS_TAB	2
#define	PSF_MODE_HAS_SEQ	4
#define	PSF_MODE_MAX		5

#define PSF2_HAS_UNICODE_TABLE 0x01

#define	PSF_TYPE_1			1
#define	PSF_TYPE_2			2
#define	PSF_TYPE_UNKNOWN	4

struct psf_font
{
	void *psf_fd;

	char psf_type;

	unsigned char psf_magic[2];
	char psf_mode;
	char psf_charsize;

	unsigned char psf2_magic[4];
	int	psf2_version;
	int	psf2_headersize;
	int	psf2_flags;
	int	psf2_length;
	int	psf2_charsize;
	int	psf2_width,psf2_height;
};

void psf_open_font(struct psf_font *font, const char *fname);
void psf_read_header(struct psf_font *font);
int psf_get_glyph_size(struct psf_font *font);
int psf_get_glyph_height(struct psf_font *font);
int psf_get_glyph_width(struct psf_font *font);
int psf_get_glyph_total(struct psf_font *font);
void psf_read_glyph(struct psf_font *font, void *mem, int size, int fill, int clear);
void psf_close_font(struct psf_font *font);

#ifdef __cplusplus
}
#endif
