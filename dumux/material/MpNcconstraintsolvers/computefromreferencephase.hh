/*****************************************************************************
 *   Copyright (C) 2011 by Andreas Lauser                                    *
 *   Institute of Hydraulic Engineering                                      *
 *   University of Stuttgart, Germany                                        *
 *   email: <givenname>.<name>@iws.uni-stuttgart.de                          *
 *                                                                           *
 *   This program is free software: you can redistribute it and/or modify    *
 *   it under the terms of the GNU General Public License as published by    *
 *   the Free Software Foundation, either version 2 of the License, or       *
 *   (at your option) any later version.                                     *
 *                                                                           *
 *   This program is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 *   GNU General Public License for more details.                            *
 *                                                                           *
 *   You should have received a copy of the GNU General Public License       *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.   *
 *****************************************************************************/
/*!
 * \file
 *
 * \brief Computes all quantities of a generic fluid state if a
 *        reference phase has been specified.
 *
 * This makes it is possible to specify just one phase and let the
 * remaining ones be calculated by the constraint solver. This
 * constraint solver assumes thermodynamic equilibrium
 */
#ifndef DUMUX_COMPUTE_FROM_REFERENCE_PHASE_HH
#define DUMUX_COMPUTE_FROM_REFERENCE_PHASE_HH

#include "../MpNcfluidstates/genericfluidstate.hh"
#include <dumux/material/MpNcconstraintsolvers/compositionfromfugacities.hh>

namespace Dumux {

/*!
 * \brief Computes all quantities of a generic fluid state if a
 *        reference phase has been specified.
 *
 * This makes it is possible to specify just one phase and let the
 * remaining ones be calculated by the constraint solver. This
 * constraint solver assumes thermodynamic equilibrium. It assumes the
 * following quantities to be set:
 *
 * - composition (mole+mass fractions) of the *reference* phase
 * - temperature of the *reference* phase
 * - saturations of *all* phases
 * - pressures of *all* phases
 *
 * after calling the solve() method the following quantities are
 * calculated in addition:
 *
 * - temperature of *all* phases
 * - density, molar density, molar volume of *all* phases
 * - composition in mole and mass fractions and molaries of *all* phases
 * - mean molar masses of *all* phases
 * - fugacity coefficients of *all* components in *all* phases
 * - if the setViscosity parameter is true, also dynamic viscosities of *all* phases
 * - if the setEnthalpy parameter is true, also specific enthalpies and internal energies of *all* phases
 */
template <class Scalar, class FluidSystem>
class ComputeFromReferencePhase
{
    enum { numPhases = FluidSystem::numPhases };
    enum { numComponents = FluidSystem::numComponents };
    typedef Dumux::CompositionFromFugacities<Scalar, FluidSystem> CompositionFromFugacities;
    typedef Dune::FieldVector<Scalar, numComponents> ComponentVector;

public:
    /*!
     * \brief Computes all quantities of a generic fluid state if a
     *        reference phase has been specified.
     *
     * This makes it is possible to specify just one phase and let the
     * remaining ones be calculated by the constraint solver. This
     * constraint solver assumes thermodynamic equilibrium. It assumes the
     * following quantities to be set:
     *
     * - composition (mole+mass fractions) of the *reference* phase
     * - temperature of the *all* phases
     * - saturations of *all* phases
     * - pressures of *all* phases
     *
     * after calling the solve() method the following quantities are
     * calculated in addition:
     *
     * - temperature of *all* phases
     * - density, molar density, molar volume of *all* phases
     * - composition in mole and mass fractions and molaries of *all* phases
     * - mean molar masses of *all* phases
     * - fugacity coefficients of *all* components in *all* phases
     * - if the setViscosity parameter is true, also dynamic viscosities of *all* phases
     * - if the setEnthalpy parameter is true, also specific enthalpies and internal energies of *all* phases
     *
     * \param mutParams The mutable parameters object which ought to be set
     * \param refPhaseIdx The phase index of the reference phase
     * \param setViscosity Specify whether the dynamic viscosity of
     *                     each phase should also be set.
     * \param setEnthalpy Specify whether the specific
     *                    enthalpy/internal energy of each phase
     *                    should also be set.
     */
    template <class FluidState, class ParameterCache>
    static void solve(FluidState &fluidState,
                      ParameterCache &paramCache,
                      int refPhaseIdx,
                      bool setViscosity,
                      bool setInternalEnergy)
    {
        ComponentVector fugVec;

        // compute the density and enthalpy of the
        // reference phase
        fluidState.updateAverageMolarMass(refPhaseIdx);
        paramCache.updatePhase(fluidState, refPhaseIdx);
        fluidState.setDensity(refPhaseIdx,
                              FluidSystem::density(fluidState, 
                                                   paramCache,
                                                   refPhaseIdx));

        if (setInternalEnergy)
            fluidState.setInternalEnergy(refPhaseIdx,
                                         FluidSystem::internalEnergy(fluidState, 
                                                                     paramCache,
                                                                     refPhaseIdx));

        if (setViscosity)
            fluidState.setViscosity(refPhaseIdx,
                                    FluidSystem::viscosity(fluidState,
                                                           paramCache,
                                                           refPhaseIdx));

        // compute the fugacities of all components in the reference phase
        for (int compIdx = 0; compIdx < numComponents; ++compIdx) {
            fluidState.setFugacityCoefficient(refPhaseIdx, 
                                              compIdx,
                                              FluidSystem::fugacityCoefficient(fluidState,
                                                                               paramCache,
                                                                               refPhaseIdx,
                                                                               compIdx));
            fugVec[compIdx] = fluidState.fugacity(refPhaseIdx, compIdx);
        }

        // compute all quantities for the non-reference phases
        for (int phaseIdx = 0; phaseIdx < numPhases; ++phaseIdx) {
            if (phaseIdx == refPhaseIdx)
                continue; // reference phase is already calculated

            CompositionFromFugacities::guessInitial(fluidState, paramCache, phaseIdx, fugVec);
            CompositionFromFugacities::solve(fluidState, paramCache, phaseIdx, fugVec);

            if (setViscosity)
                fluidState.setViscosity(phaseIdx,
                                        FluidSystem::viscosity(fluidState, 
                                                               paramCache, 
                                                               phaseIdx));
            
            if (setInternalEnergy)
                fluidState.setInternalEnergy(phaseIdx,
                                             FluidSystem::internalEnergy(fluidState,
                                                                         paramCache,
                                                                         phaseIdx));
        }
    };
};

} // end namespace Dumux

#endif