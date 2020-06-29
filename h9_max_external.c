/**
	@file
	h9_max_external - An Eventide H9 control object as a Max external
	daniel collins - malacalypse@argx.net

	@ingroup	malacalypse
*/

#include "ext.h"							// standard Max include, always required
#include "ext_obex.h"						// required for new style Max object

#include "midi_parser.h"
#include "h9.h"

////////////////////////// object struct

typedef struct _h9_external {
    t_object    ob;
    t_symbol    *name;
    midi_parser *parser;

    // Inlets and outlets
    void *m_knob1;
    void *m_knob2;
    void *m_knob3;
    void *m_knob4;
    void *m_knob5;
    void *m_knob6;
    void *m_knob7;
    void *m_knob8;
    void *m_knob9;
    void *m_knob10;
    void *m_expr;
    void *m_psw;

    void *m_outlet_sysex;
    void *m_outlet_enabled;

    // Configuration
    uint8_t sysex_id;
    uint8_t midi_channel;
} t_h9_external;

///////////////////////// function prototypes
//// standard set
void *h9_external_new(t_symbol *s, long argc, t_atom *argv);
void h9_external_free(t_h9_external *x);
void h9_external_assist(t_h9_external *x, void *b, long m, long a, char *s);

void h9_external_bang(t_h9_external *x);
void h9_external_identify(t_h9_external *x);
void h9_external_set(t_h9_external *x, t_symbol *s, long ac, t_atom *av);
void h9_external_int(t_h9_external *x, long n);

static void h9_external_cc(midi_parser *parser, uint8_t channel, uint8_t cc, uint8_t value);
static void h9_external_sysex(midi_parser *parser, uint8_t *sysex, size_t len);

//////////////////////// global class pointer variable
void *h9_external_class;

void ext_main(void *r)
{
	t_class *c;

	c = class_new("h9", (method)h9_external_new, (method)h9_external_free, (long)sizeof(t_h9_external),
				  0L /* leave NULL!! */, A_GIMME, 0);

    // Declare the responding methods for various type handlers
	class_addmethod(c, (method)h9_external_bang,			"bang", 0);
	class_addmethod(c, (method)h9_external_int,			"int",		A_LONG, 0);
	class_addmethod(c, (method)h9_external_identify,		"identify", 0);
    class_addmethod(c, (method)h9_external_set,          "set", A_GIMME, 0);
    CLASS_METHOD_ATTR_PARSE(c, "identify", "undocumented", gensym("long"), 0, "1");

//    // Knob, Expr, and PSW inputs
//    class_addmethod(c, (method)h9_knob1, "in1", A_LONG, 0);
//    class_addmethod(c, (method)h9_knob2, "in2", A_LONG, 0);
//    class_addmethod(c, (method)h9_knob3, "in3", A_LONG, 0);
//    class_addmethod(c, (method)h9_knob4, "in4", A_LONG, 0);
//    class_addmethod(c, (method)h9_knob5, "in5", A_LONG, 0);
//    class_addmethod(c, (method)h9_knob6, "in6", A_LONG, 0);
//    class_addmethod(c, (method)h9_knob7, "in7", A_LONG, 0);
//    class_addmethod(c, (method)h9_knob8, "in8", A_LONG, 0);
//    class_addmethod(c, (method)h9_knob9, "in9", A_LONG, 0);
//    class_addmethod(c, (method)h9_knob10, "in10", A_LONG, 0);
//    class_addmethod(c, (method)h9_expr, "in11", A_LONG, 0);
//    class_addmethod(c, (method)h9_psw, "in12", A_LONG, 0);

	/* you CAN'T call this from the patcher */
	class_addmethod(c, (method)h9_external_assist,			"assist",		A_CANT, 0);

	CLASS_ATTR_SYM(c, "name", 0, t_h9_external, name);

	class_register(CLASS_BOX, c);
	h9_external_class = c;
}

void *h9_external_new(t_symbol *s, long argc, t_atom *argv)
{
    t_h9_external *x = NULL;

    if ((x = (t_h9_external *)object_alloc(h9_external_class))) {
        x->name = gensym("");
        if (argc && argv) {
            x->name = atom_getsym(argv);
        }
        if (!x->name || x->name == gensym(""))
            x->name = symbol_unique();

        x->m_outlet_enabled = intout((t_object *)x);
        x->m_outlet_sysex = listout((t_object *)x);

        // Init the zero state of the object
        x->sysex_id = 1; // not 0
        x->midi_channel = 0;

        // Setup the MIDI parsing
        x->parser = midi_parser_new((void *)x, h9_external_sysex, h9_external_cc);
        if (x->parser != NULL) {
            midi_parser_filter_channel(x->parser, x->midi_channel);
            uint8_t preamble[] = { SYSEX_EVENTIDE, SYSEX_H9, x->sysex_id };
            midi_parser_filter_sysex(x->parser, preamble, 3);
            object_post((t_object *)x, "Whoop! Let's make it do! 2");
        }
    }

    return (x);
}


void h9_external_assist(t_h9_external *x, void *b, long m, long a, char *s)
{
	if (m == ASSIST_INLET) { //inlet
        switch(a) {
            case 0:
                sprintf(s, "I am the MASTER");
                break;
            default:
                sprintf(s, "I am inlet %ld", a);
        }
	}
	else {	// outlet
        switch(a) {
            case 0:
                sprintf(s, "I am outlet ZERO");
                break;
            default:
                sprintf(s, "I am outlet %ld", a);
        }
	}
}

void h9_external_free(t_h9_external *x)
{
    if (x->parser != NULL) {
        midi_parser_delete(x->parser);
        free(x->parser);
    }
}

// Input handlers for each message

void h9_external_int(t_h9_external *x, long n)
{
    object_post((t_object *)x, "Processing int %ld", n);
    if (x->parser != NULL) {
        midi_parse(x->parser, (uint8_t)n);
    }
}

void h9_external_set(t_h9_external *x, t_symbol *s, long argc, t_atom *argv)
{
    long i;
    t_atom *ap;
    t_symbol *sym;

    // Quick little tool to help decipher the arguments
    post("message selector is %s",s->s_name);
    post("there are %ld arguments",argc);
    // increment ap each time to get to the next atom
    for (i = 0, ap = argv; i < argc; i++, ap++) {
        switch (atom_gettype(ap)) {
            case A_LONG:
                post("%ld: %ld",i+1,atom_getlong(ap));
                break;
            case A_FLOAT:
                post("%ld: %.2f",i+1,atom_getfloat(ap));
                break;
            case A_SYM:
                sym = atom_getsym(ap);
                post("%ld: %s",i+1, sym->s_name);
                if (sym == gensym("xyzzy")) {
                    object_post((t_object *)x, "A hollow voice says 'Plugh'");
                }
                break;
            default:
                post("%ld: unknown atom type (%ld)", i+1, atom_gettype(ap));
                break;
        }
    }
}

void h9_external_bang(t_h9_external *x)
{
    // Ordinarily, this would dump the current state to sysex. For now, that's not yet ready.
    object_post((t_object *)x, "%s says \"Bang!\"", x->name->s_name);
}

void h9_external_identify(t_h9_external *x)
{
	object_post((t_object *)x, "Hello, my name is %s", x->name->s_name);
}

static void h9_external_cc(midi_parser *parser, uint8_t channel, uint8_t cc, uint8_t value) {
    t_h9_external *h9 = (t_h9_external *)parser->context;
    object_post((t_object *)h9, "%s received cc %u on channel %u with value %u", h9->name->s_name, cc, channel, value);
}

static void h9_external_sysex(midi_parser *parser, uint8_t *sysex, size_t len) {
    t_h9_external *h9 = (t_h9_external *)parser->context;
    object_post((t_object *)h9, "%s received sysex %.*s", h9->name->s_name, len, sysex);
    t_atom bytes[len];
    t_max_err result = atom_setchar_array(len, bytes, len, sysex);
    if (result == MAX_ERR_NONE) {
        outlet_list(h9->m_outlet_sysex, NULL, len, bytes);
        object_post((t_object *)h9, "%s dumped sysex.", h9->name->s_name);
    }
}
