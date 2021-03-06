/*
Copyright (c) 2014, J.D. Koftinoff Software, Ltd.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "avdecc-cmd.h"
#include "raw.h"
#include "aecp-cmd.h"
#include "aecp.h"

int main( int argc, char **argv )
{
    int r = 1;
    uint16_t sequence_id = 0;
    struct jdksavdecc_eui48 destination_mac;
    struct jdksavdecc_eui64 target_entity_id;
    uint16_t descriptor_type = 0;
    uint16_t descriptor_index = 0;
    int arg = 0;

    if ( argc < 8 )
    {
        fprintf( stderr,
                 "avdecc-read-descriptor usage:\n"
                 "\tavdecc-read-descriptor [verbosity] [timeout_in_ms] [network_port] [sequence_id] [destination_mac] "
                 "[target_entity_id] [descriptor_type] [descriptor_index]\n\n" );
        return 1;
    }

    if ( argc > ++arg )
    {
        arg_verbose = atoi( argv[arg] );
    }
    if ( argc > ++arg )
    {
        arg_time_in_ms_to_wait = atoi( argv[arg] );
    }
    if ( argc > ++arg )
    {
        arg_network_port = argv[arg];
    }
    if ( argc > ++arg )
    {
        arg_sequence_id = argv[arg];
    }
    if ( argc > ++arg )
    {
        arg_destination_mac = argv[arg];
    }
    if ( argc > ++arg )
    {
        arg_target_entity_id = argv[arg];
    }
    if ( argc > ++arg )
    {
        arg_descriptor_type = argv[arg];
    }
    if ( argc > ++arg )
    {
        arg_descriptor_index = argv[arg];
    }

    sequence_id = (uint16_t)strtol( arg_sequence_id, 0, 0 );
    if ( arg_destination_mac )
    {
        jdksavdecc_eui48_init_from_cstr( &destination_mac, arg_destination_mac );
    }

    if ( arg_target_entity_id )
    {
        jdksavdecc_eui64_init_from_cstr( &target_entity_id, arg_target_entity_id );
    }
    else
    {
        bzero( &target_entity_id, sizeof( target_entity_id ) );
    }

    descriptor_index = (uint16_t)strtol( arg_descriptor_index, 0, 0 );

    if ( !jdksavdecc_get_uint16_value_for_name( jdksavdecc_aem_print_descriptor_type, arg_descriptor_type, &descriptor_type ) )
    {
        errno = 0;
        char *end = (char *)arg_descriptor_type;
        if ( arg_descriptor_type )
        {
            descriptor_type = (uint16_t)strtol( arg_descriptor_type, &end, 0 );
        }
        if ( !arg_descriptor_type || errno == ERANGE || *end )
        {
            struct jdksavdecc_uint16_name *name = jdksavdecc_aem_print_descriptor_type;
            fprintf( stderr, "Invalid AECP AEM descriptor type. Options are:\n" );
            while ( name->name )
            {
                fprintf( stdout, "\t0x%04x %s\n", name->value, name->name );
                name++;
            }
            return 1;
        }
    }

    {
        struct raw_context net;
        int fd = raw_socket( &net, JDKSAVDECC_AVTP_ETHERTYPE, arg_network_port, jdksavdecc_multicast_adp_acmp.value );
        if ( fd >= 0 )
        {
            struct jdksavdecc_aem_command_read_descriptor pdu;
            struct jdksavdecc_frame frame;
            struct jdksavdecc_eui64 zero;
            jdksavdecc_frame_init( &frame );
            bzero( &pdu, sizeof( pdu ) );
            bzero( &zero, sizeof( zero ) );
            memcpy( frame.src_address.value, net.m_my_mac, 6 );

            if ( aecp_aem_form_read_descriptor_command(
                     &frame, &pdu, sequence_id, destination_mac, target_entity_id, descriptor_type, descriptor_index ) == 0 )
            {
                if ( raw_send( &net, frame.dest_address.value, frame.payload, frame.length ) > 0 )
                {
                    if ( arg_verbose > 0 )
                    {
                        fprintf( stdout, "Sent:\n" );
                        aecp_aem_print( stdout, &frame, &pdu.aem_header );

                        if ( arg_verbose > 1 )
                        {
                            avdecc_cmd_print_frame_payload( stdout, &frame );
                        }
                    }
                    r = 0;

                    avdecc_cmd_process_incoming_raw( &pdu, &net, arg_time_in_ms_to_wait, aecp_aem_process );
                }
            }
            raw_close( &net );
        }
        else
        {
            fprintf( stderr, "avdecc-read-descriptor: unable to open port '%s'\n", arg_network_port );
        }
    }
    return r;
}
