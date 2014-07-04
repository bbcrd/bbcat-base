/*
 * SOFA.cpp
 *
 *  Created on: Jun 18, 2014
 *      Author: chrisp
 */

#include "SOFA.h"

USE_BBC_AUDIOTOOLBOX

SOFA::SOFA(const std::string filename) :
	sofa_file(filename.c_str(), sofa_file_t::read),
	sample_rate(0.f)
{
	try
	{
		// Open the file and check to make sure it's valid.
		DEBUG("Creating SOFA with filename: %s", filename.c_str());
		// try some calls to check BadId error
		DEBUG("SOFA name: %s",sofa_file.getName().c_str());


		sofa_att_collection_t sofa_atts = sofa_file.getAtts();

		// check it's a SOFA file
		sofa_att_t conventions = get_att("Conventions");
		if (!conventions.isNull())
		{
			std::string convention_name;
			conventions.getValues(convention_name);
			if (convention_name != "SOFA")
			{
				ERROR("This is not a SOFA file!");
			}
		}

		// get sofa dimensions
		sofa_dims.N = sofa_file.getDim("N");
		if(sofa_dims.N.isNull()) ERROR("SOFA dim N is null");
		sofa_dims.M = sofa_file.getDim("M");
		if(sofa_dims.M.isNull()) ERROR("SOFA dim M is null");
		sofa_dims.R = sofa_file.getDim("R");
		if(sofa_dims.R.isNull()) ERROR("SOFA dim R is null");
		sofa_dims.E = sofa_file.getDim("E");
		if(sofa_dims.E.isNull()) ERROR("SOFA dim E is null");
		sofa_dims.C = sofa_file.getDim("C");
		if(sofa_dims.C.isNull()) ERROR("SOFA dim C is null");
		sofa_dims.I = sofa_file.getDim("I");
		if(sofa_dims.I.isNull()) ERROR("SOFA dim I is null");

		// explicitly check and report SOFA Convention
		sofa_att_t sofa_convention = get_att("SOFAConventions");
		if (!sofa_convention.isNull())
		{
			sofa_convention.getValues(convention_name);
			DEBUG("Convention: %s", convention_name.c_str());
		}

		sofa_var_t sr_var = get_var("Data.SamplingRate");
		if (!sr_var.isNull())
		{
			sr_var.getVar(&sample_rate);
			std::string sr_units;
			sr_var.getAtt("Units").getValues(sr_units);
			DEBUG("Sample rate is: %f %s", sample_rate, sr_units.c_str());
		}
		else
		{
			ERROR("Sample rate variable is null!!!!");
		}
	}
	catch (netCDF::exceptions::NcException& e)
	{
		ERROR("NetCDF error: %s", e.what());
	}
}

SOFA::~SOFA()
{
}

float SOFA::get_samplerate() const
{
	return sample_rate;
}

size_t SOFA::get_num_measurements() const
{
	return sofa_dims.M.getSize();
}

size_t SOFA::get_ir_length() const
{
	return sofa_dims.N.getSize();
}

size_t SOFA::get_num_receivers() const
{
	return sofa_dims.R.getSize();
}

size_t SOFA::get_num_emitters() const
{
	return sofa_dims.E.getSize();
}

/**
 * This gets an IR measurement for a given receiver with all samples,
 * for at least SimpleFreeFieldHRIR and MultiSpeakerBRIR.
 */
bool SOFA::get_ir(SOFA::audio_buffer_t& ir_buffer, uint_t indexM, uint_t indexR, uint_t indexE) const
{
	sofa_var_t ir_data = get_var("Data.IR");
	if (ir_data.isNull()) return false;
	size_t n_dims = ir_data.getDimCount();

	get_index_vec_t start(n_dims,0);
	get_index_vec_t count(n_dims,1);
	if (indexM >= sofa_dims.M.getSize() || indexR >= sofa_dims.R.getSize() || indexE >= sofa_dims.E.getSize())
	{
		ERROR("Index out of range.");
		return false;
	}
	// set start and count indices
	start[0] = indexM;
	start[1] = indexR;
	if (n_dims > 3) start[2] = indexE; // for MultiSpeakerBRIR
	count[n_dims-1] = sofa_dims.N.getSize();

	return get_ir_data(start, count, ir_buffer);
}

/**
 * Get a vector of delays for a particular receiver emitter combo
 */
bool SOFA::get_delays(SOFA::delay_buffer_t& delays, uint_t indexR, uint_t indexE) const
{
	sofa_var_t delay_data = get_var("Data.Delay");
	if (delay_data.isNull()) return false;
	size_t n_dims = delay_data.getDimCount();

	get_index_vec_t start(n_dims,0);
	get_index_vec_t count(n_dims,1);
	if (indexR >= sofa_dims.R.getSize() || indexE >= sofa_dims.E.getSize())
	{
		ERROR("Index out of range.");
		return false;
	}
	// set start and count indices
	start[1] = indexR;
	if (n_dims > 2) start[1] = indexE; // for MultiSpeakerBRIR
	if (delay_data.getDims()[0].getName() == "M")
	{
		count[0] = sofa_dims.M.getSize();
	}
	return get_delay_data(start, count, delays);
}

SOFA::positions_array_t SOFA::get_source_positions() const
{
	return get_position_var_data("SourcePosition");
}

SOFA::positions_array_t SOFA::get_emitter_positions() const
{
	return get_position_var_data("EmitterPosition");
}

SOFA::positions_array_t SOFA::get_listener_positions() const
{
	return get_position_var_data("ListenerPosition");
}

SOFA::positions_array_t SOFA::get_receiver_positions() const
{
	return get_position_var_data("ReceiverPosition");
}

SOFA::positions_array_t SOFA::get_listener_view_vecs() const
{
	return get_position_var_data("ListenerView");
}

SOFA::positions_array_t SOFA::get_listener_up_vecs() const
{
	return get_position_var_data("ListenerUp");
}

void SOFA::list_vars() const
{
	sofa_var_collection_t sofa_vars = sofa_file.getVars();
	// loop through variables and print name
	std::cout << "SOFA variables are:" << std::endl;
	for (sofa_var_collection_t::iterator var_it = sofa_vars.begin(); var_it != sofa_vars.end(); var_it++)
	{
		std::cout << "\t" << (*var_it).first << " (" << (*var_it).second.getType().getName() << ")" << std::endl;
		sofa_dims_vec_t var_dims = (*var_it).second.getDims();
		std::cout << "\t\tDims: ";
		for (sofa_dims_vec_t::iterator dims_it = var_dims.begin(); dims_it != var_dims.end(); dims_it++)
		{
			std::cout << (*dims_it).getName() << " (" << (*dims_it).getSize() << "), ";
		}
		std::cout << std::endl;
	}
}

void SOFA::list_atts() const
{
	sofa_att_collection_t sofa_atts = sofa_file.getAtts();
	// loop through variables and print name
	std::cout << "SOFA attributes are:" << std::endl;
	for (sofa_att_collection_t::iterator att_it = sofa_atts.begin(); att_it != sofa_atts.end(); att_it++)
	{
		std::cout << "\t" << (*att_it).first << " (" << (*att_it).second.getType().getName() << ")" << std::endl;
		std::string att_val;
		(*att_it).second.getValues(att_val);
		std::cout << "\t\t" << att_val << std::endl;
	}
}

void SOFA::list_varatts(const sofa_var_t sofa_var) const
{
	sofa_varatt_collection_t sofa_atts = sofa_var.getAtts();
	// loop through variables and print name
	DEBUG2(sofa_var.getName() << " attributes are:");
	for (sofa_varatt_collection_t::iterator att_it = sofa_atts.begin(); att_it != sofa_atts.end(); att_it++)
	{
		DEBUG2("\t" << (*att_it).first << " (" << (*att_it).second.getType().getName() << ")");
		std::string att_val;
		(*att_it).second.getValues(att_val);
		DEBUG2("\t\t" << att_val);
	}
}

SOFA::sofa_var_t SOFA::get_var(const std::string var_name) const
{
	sofa_var_collection_t sofa_vars = sofa_file.getVars();
	sofa_var_collection_t::iterator var_search = sofa_vars.find(var_name);
	if (var_search == sofa_vars.end())
	{
		ERROR("%s variable not found!",var_name.c_str());
		return sofa_var_t();
	}
	else
	{
		DEBUG("Reading variable: %s", var_name.c_str());
		return (*var_search).second;
	}
}

SOFA::sofa_att_t SOFA::get_att(const std::string att_name) const
{
	sofa_att_collection_t sofa_atts = sofa_file.getAtts();
	sofa_att_collection_t::iterator att_search = sofa_atts.find(att_name);
	if (att_search == sofa_atts.end())
	{
		ERROR("%s variable not found!", att_name.c_str());
		return sofa_att_t();
	}
	else
	{
		DEBUG("Reading attribute: %s", att_name.c_str());
		return (*att_search).second;
	}
}

bool SOFA::get_ir_data(get_index_vec_t start, get_index_vec_t count, SOFA::audio_buffer_t& ir_buffer) const
{
	sofa_var_t ir_data = get_var("Data.IR");
	if (ir_data.isNull())
	{
		return false;
	}
	size_t size = 1;
	if (ir_data.getDimCount() < 0) ERROR("Dim count is <0?!");
	for (uint_t ii = 0; ii < static_cast<uint_t>(ir_data.getDimCount()); ii ++)
	{
		size *= count[ii];
	}
	if (ir_buffer.size() != size)
	{
		DEBUG("Warning: resizing ir_buffer.");
		ir_buffer.resize(size);
	}
	ir_data.getVar(start,count,&ir_buffer[0]);
	return true;
}

bool SOFA::get_delay_data(get_index_vec_t start, get_index_vec_t count, SOFA::delay_buffer_t& delays) const
{
	sofa_var_t delay_data = get_var("Data.Delay");
	if (delay_data.isNull())
	{
		return false;
	}
	size_t size = 1;
	if (delay_data.getDimCount() < 0) ERROR("Dim count is <0?!");
	for (uint_t ii = 0; ii < static_cast<uint_t>(delay_data.getDimCount()); ii ++)
	{
		size *= count[ii];
	}
	if (delays.size() != size)
	{
		DEBUG("Warning: resizing delays buffer.");
		delays.resize(size);
	}
	delay_data.getVar(start,count,&delays[0]);
	return true;
}

SOFA::positions_array_t SOFA::get_position_var_data(const std::string position_variable_name) const
{
	// Listener View
	sofa_var_t pos_var = get_var(position_variable_name);
	if (!pos_var.isNull())
	{
		list_varatts(pos_var);

		sofa_dims_vec_t var_dims = pos_var.getDims();
		if (var_dims[1].getSize() != 3)
		{
			ERROR("Coordinate dimension wrong");
			return positions_array_t();
		}

		positions_array_t pos_var_data(var_dims[0].getSize());
		float pos_raw[var_dims[0].getSize()*var_dims[1].getSize()];
		DEBUG3("Dims: [" << var_dims[0].getSize() << ", " << var_dims[1].getSize() << "]");

		// get coordinate type (cartesian or spherical)
		// this isn't valid for look and up vectors
		std::string coord_type;
		if (!pos_var.getAtts().empty())
		{
			pos_var.getAtt("Type").getValues(coord_type);
		}

		// get data
		const get_index_vec_t start(pos_var.getDimCount(),0);
		get_index_vec_t count(pos_var.getDimCount(), 1);
		count[0] = var_dims[0].getSize();
		count[1] = var_dims[1].getSize();
		pos_var.getVar(start,count,pos_raw);

		// create array of Position objects
		for (uint_t ii = 0; ii < count[0]; ii++)
		{
			pos_var_data[ii].pos.x = pos_raw[ii*count[1]];
			pos_var_data[ii].pos.y = pos_raw[ii*count[1] + 1];
			pos_var_data[ii].pos.z = pos_raw[ii*count[1] + 2];
			pos_var_data[ii].polar = (coord_type == "spherical");
		}

		return pos_var_data;
	}
	else
	{
		ERROR("ListenerView not found, error in SOFA file.");
		return positions_array_t();
	}
}