/******************************************************************************
* Copyright (c) 2011, Michael P. Gerlek (mpg@flaxen.com)
*
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following
* conditions are met:
*
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in
*       the documentation and/or other materials provided
*       with the distribution.
*     * Neither the name of Hobu, Inc. or Flaxen Geo Consulting nor the
*       names of its contributors may be used to endorse or promote
*       products derived from this software without specific prior
*       written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
* "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
* LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
* FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
* COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
* INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
* BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
* OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
* AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
* OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
* OF SUCH DAMAGE.
****************************************************************************/

#include "FauxReader.hpp"

#include <pdal/Options.hpp>
#include <pdal/PointView.hpp>
#include <pdal/pdal_macros.hpp>

#include <ctime>

namespace pdal
{

static PluginInfo const s_info = PluginInfo(
    "readers.faux",
    "Faux Reader",
    "http://pdal.io/stages/readers.faux.html" );

CREATE_STATIC_PLUGIN(1, 0, FauxReader, Reader, s_info)

std::string FauxReader::getName() const { return s_info.name; }

static Mode string2mode(const std::string& str)
{
    std::string lstr = Utils::tolower(str);
    if (lstr == "constant")
        return Constant;
    if (lstr == "random")
        return Random;
    if (lstr == "ramp")
        return Ramp;
    if (lstr == "uniform")
        return Uniform;
    if (lstr == "normal")
        return Normal;
    std::ostringstream oss;
    oss << s_info.name << ": Invalid 'mode' option: '" << str << "'.";
    throw pdal_error(oss.str());
}


FauxReader::FauxReader()
    : pdal::Reader()
{
    m_count = 0;
}

void FauxReader::processOptions(const Options& options)
{
    BOX3D bounds;
    try
    {
        bounds = options.getValueOrDefault<BOX3D>("bounds",
            BOX3D(0, 0, 0, 1, 1, 1));
    }
    catch (Option::cant_convert)
    {
        std::string s = options.getValueOrDefault<std::string>("bounds");

        std::ostringstream oss;
        oss << getName() << ": Invalid 'bounds' specification: '" << s <<
            "'.  Format: '([xmin,xmax],[ymin,ymax],[zmin,zmax])'.";
        throw pdal_error(oss.str());
    }
    m_minX = bounds.minx;
    m_maxX = bounds.maxx;
    m_minY = bounds.miny;
    m_maxY = bounds.maxy;
    m_minZ = bounds.minz;
    m_maxZ = bounds.maxz;

    // For backward compatibility.
    if (m_count == 0)
        m_count = options.getValueOrThrow<point_count_t>("num_points");
    if (m_count == 0)
    {
        std::ostringstream oss;
        oss << getName() << ": Option 'count' must be non-zero.";
        throw pdal_error(oss.str());
    }

    m_mean_x = options.getValueOrDefault<double>("mean_x",0.0);
    m_mean_y = options.getValueOrDefault<double>("mean_y",0.0);
    m_mean_z = options.getValueOrDefault<double>("mean_z",0.0);
    m_stdev_x = options.getValueOrDefault<double>("stdev_x",1.0);
    m_stdev_y = options.getValueOrDefault<double>("stdev_y",1.0);
    m_stdev_z = options.getValueOrDefault<double>("stdev_z",1.0);
    m_mode = string2mode(options.getValueOrThrow<std::string>("mode"));
    m_numReturns = options.getValueOrDefault("number_of_returns", 0);
    if (m_numReturns > 10)
    {
        std::ostringstream oss;
        oss << getName() << ": Option 'number_of_returns' must be in the range "
            "[0,10].";
        throw pdal_error(oss.str());
    }
    if (m_count > 1)
    {
        m_delX = (m_maxX - m_minX) / (m_count - 1);
        m_delY = (m_maxY - m_minY) / (m_count - 1);
        m_delZ = (m_maxZ - m_minZ) / (m_count - 1);
    }
    else
    {
        m_delX = 0;
        m_delY = 0;
        m_delZ = 0;
    }
}


Options FauxReader::getDefaultOptions()
{
    Options options;
    Option count("num_points", 10, "Number of points");
    options.add(count);
    return options;
}


void FauxReader::addDimensions(PointLayoutPtr layout)
{
    layout->registerDims(getDefaultDimensions());
    if (m_numReturns > 0)
    {
        layout->registerDim(Dimension::Id::ReturnNumber);
        layout->registerDim(Dimension::Id::NumberOfReturns);
    }
}


Dimension::IdList FauxReader::getDefaultDimensions()
{
    Dimension::IdList ids;

    ids.push_back(Dimension::Id::X);
    ids.push_back(Dimension::Id::Y);
    ids.push_back(Dimension::Id::Z);
    ids.push_back(Dimension::Id::OffsetTime);
    return ids;
}

void FauxReader::ready(PointTableRef /*table*/)
{
    m_returnNum = 1;
    m_time = 0;
    m_seed = (uint32_t)std::time(NULL);
    m_index = 0;
}


bool FauxReader::processOne(PointRef& point)
{
    double x(0);
    double y(0);
    double z(0);

    if (m_index >= m_count)
        return false;

    switch (m_mode)
    {
    case Random:
        x = Utils::random(m_minX, m_maxX);
        y = Utils::random(m_minY, m_maxY);
        z = Utils::random(m_minZ, m_maxZ);
        break;
    case Constant:
        x = m_minX;
        y = m_minY;
        z = m_minZ;
        break;
    case Ramp:
        x = m_minX + m_delX * m_index;
        y = m_minY + m_delY * m_index;
        z = m_minZ + m_delZ * m_index;
        break;
    case Uniform:
        x = Utils::uniform(m_minX, m_maxX, m_seed++);
        y = Utils::uniform(m_minY, m_maxY, m_seed++);
        z = Utils::uniform(m_minZ, m_maxZ, m_seed++);
        break;
    case Normal:
        x = Utils::normal(m_mean_x, m_stdev_x, m_seed++);
        y = Utils::normal(m_mean_y, m_stdev_y, m_seed++);
        z = Utils::normal(m_mean_z, m_stdev_z, m_seed++);
        break;
    }

    point.setField(Dimension::Id::X, x);
    point.setField(Dimension::Id::Y, y);
    point.setField(Dimension::Id::Z, z);
    point.setField(Dimension::Id::OffsetTime, m_time++);
    if (m_numReturns > 0)
    {
        point.setField(Dimension::Id::ReturnNumber, m_returnNum);
        point.setField(Dimension::Id::NumberOfReturns, m_numReturns);
        m_returnNum = (m_returnNum % m_numReturns) + 1;
    }
    m_index++;
    return true;
}


point_count_t FauxReader::read(PointViewPtr view, point_count_t count)
{
    for (PointId idx = 0; idx < count; ++idx)
    {
        PointRef point = view->point(idx);
        if (!processOne(point))
            break;
        if (m_cb)
            m_cb(*view, idx);
    }
    return count;
}

} // namespace pdal
