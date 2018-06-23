#pragma once

#include <percy/chain.hpp>

namespace percy
{
    /***************************************************************************
        Functions to convert a chain to the output format for the IWLS 2018
        programming competition.
    ***************************************************************************/
    void 
    to_iwls(const chain& c, std::ostream& s)
    {
        assert(c.get_nr_outputs() == 1);
        const auto nr_inputs = c.get_nr_inputs();

        for (int i = 0; i < c.get_nr_steps(); i++) {
            auto step = c.get_step(i);
            s << std::endl;
            assert(nr_inputs + i >= nr_inputs) ; 
            s << char(('A' + nr_inputs + i)); 
            s << " = "; 

            kitty::print_binary(c.get_operator(i), s);
            s << " "; 
            for (int k = 0; k < c.get_fanin(); k++) {
                if (step[k] >= nr_inputs)
                    s << char(('A' + step[k]));
                else
                    s << char(('a' + step[k]));
            }
        }
    }

}

