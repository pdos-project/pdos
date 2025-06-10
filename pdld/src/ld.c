/******************************************************************************
 * @file            ld.c
 *
 * Released to the public domain.
 *
 * Anyone and anything may copy, edit, publish, use, compile, sell and
 * distribute this work and all its parts in any form for any purpose,
 * commercial and non-commercial, without any restrictions, without
 * complying with any conditions and by any means.
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ld.h"
#include "xmalloc.h"

const char *program_name = NULL;

static struct ld_state ld_state_s = {0};
struct ld_state *ld_state = &ld_state_s;

int main (int argc, char **argv)
{
    char **input_filenames;
    int i;

    if (argc && *argv) {
        program_name = *argv;
    }

    ld_state->output_filename = "a.exe";
    ld_state->oformat = LD_OFORMAT_COFF;

    input_filenames = ld_parse_args (argc, argv, 1);
    if (input_filenames == NULL) {
        if (ld_state->no_input_files_is_fine) exit (EXIT_SUCCESS);
        ld_fatal_error ("no input files");
    }
        
    symbols_init ();

    if (ld_state->oformat == LD_OFORMAT_AOUT) aout_init ();
    else if (ld_state->oformat == LD_OFORMAT_ATARI) atari_init ();

    for (i = 0; i < argc; i++) {
        if (input_filenames[i]) read_input_file (input_filenames[i]);
    }

    sections_destroy_empty_before_collapse ();

    if (ld_state->oformat == LD_OFORMAT_COFF) {
        coff_before_link ();
    } else if (ld_state->oformat == LD_OFORMAT_ELF) {
        elf_before_link ();
    } else if (ld_state->oformat == LD_OFORMAT_LX) {
        lx_before_link ();
    }
    
    link ();

    if (ld_state->oformat == LD_OFORMAT_COFF) coff_after_link ();
    
    if (ld_state->oformat == LD_OFORMAT_AMIGA) {
        amiga_write (ld_state->output_filename);
    } else if (ld_state->oformat == LD_OFORMAT_AOUT) {
        aout_write (ld_state->output_filename);
    } else if (ld_state->oformat == LD_OFORMAT_ATARI) {
        atari_write (ld_state->output_filename);
    } else if (ld_state->oformat == LD_OFORMAT_BINARY) {
        binary_write (ld_state->output_filename);
    } else if (ld_state->oformat == LD_OFORMAT_CMS) {
        cms_write (ld_state->output_filename);
    } else if (ld_state->oformat == LD_OFORMAT_COFF) {
        coff_write (ld_state->output_filename);
    } else if (ld_state->oformat == LD_OFORMAT_ELF) {
        elf_write (ld_state->output_filename);
    } else if (ld_state->oformat == LD_OFORMAT_LX) {
        lx_write (ld_state->output_filename);
    } else if (ld_state->oformat == LD_OFORMAT_MVS) {
        mvs_write (ld_state->output_filename);
    } else if (ld_state->oformat == LD_OFORMAT_VSE) {
        vse_write (ld_state->output_filename);
    }
    
    if (ld_state->output_map_filename) map_write (ld_state->output_map_filename);
    
    sections_destroy ();
    symbols_destroy ();

    free (input_filenames);

    if (ld_get_error_count ()) {
        remove (ld_state->output_filename);
        exit (EXIT_FAILURE);
    }
    
    return EXIT_SUCCESS;
}
