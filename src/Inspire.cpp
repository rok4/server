/*
 * Copyright © (2011-2013) Institut national de l'information
 *                    géographique et forestière
 *
 * Géoportail SAV <contact.geoservices@ign.fr>
 *
 * This software is a computer program whose purpose is to publish geographic
 * data using OGC WMS and WMTS protocol.
 *
 * This software is governed by the CeCILL-C license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL-C
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 *
 * knowledge of the CeCILL-C license and that you accept its terms.
 */

/**
 * \file Inspire.cpp
 ** \~french
 * \brief Implémentation du namespace Inspire, gérant les contraintes Inspire
 ** \~english
 * \brief Implement and the namespace Inspire, handling inspire constraints
 */

#include <string>
#include <vector>
#include <algorithm>

#include "Inspire.h"

namespace Inspire {

/**
 * \~french \brief Liste des noms de couche Inspire
 * \details Cette liste est définie ici : https://inspire.ec.europa.eu/layer/
 * Il est possible de la récupérer la liste brute avec la commande suivante : 
 * \code{.sh}
 * curl -o- --silent "https://inspire.ec.europa.eu/layer/layer.fr.json" | jq .register.containeditems[].layer.layername.text
 * \endcode
 * \~english \brief Inspire layer names list
 * \details List is define here : https://inspire.ec.europa.eu/layer/
 * Possibility to get the raw list with the command : 
 * \code{.sh}
 * curl -o- --silent "https://inspire.ec.europa.eu/layer/layer.fr.json" | jq .register.containeditems[].layer.layername.text
 * \endcode
 */
const std::vector<std::string> layer_names = {
    "AD.Address", "AF.AgriculturalHolding", "AF.AquacultureHolding", "AF.Site", "AM._ZoneTypeCode_", 
    "AM.AirQualityManagementZone", "AM.AnimalHealthRestrictionZone", "AM.AreaForDisposalOfWaste", 
    "AM.BathingWaters", "AM.CoastalZoneManagementArea", "AM.DesignatedWaters", "AM.DrinkingWaterProtectionArea", 
    "AM.FloodUnitOfManagement", "AM.ForestManagementArea", "AM.MarineRegion", "AM.NitrateVulnerableZone", 
    "AM.NoiseRestrictionZone", "AM.PlantHealthProtectionZone", "AM.ProspectingAndMiningPermitArea", 
    "AM.RegulatedFairwayAtSeaOrLargeInlandWater", "AM.RestrictedZonesAroundContaminatedSites", "AM.RiverBasinDistrict", 
    "AM.SensitiveArea", "AM.WaterBodyForWFD", "AU._MaritimeZoneTypeValue_", "AU.AdministrativeBoundary", "AU.AdministrativeUnit", 
    "AU.Baseline", "AU.Condominium", "AU.ContiguousZone", "AU.ContinentalShelf", "AU.ExclusiveEconomicZone", "AU.InternalWaters", 
    "AU.MaritimeBoundary", "AU.TerritorialSea", "BR.Bio-geographicalRegion", "BU.Building", "BU.BuildingPart", "CP.CadastralBoundary", 
    "CP.CadastralParcel", "CP.CadastralZoning", "EF.EnvironmentalMonitoringFacilities", "EF.EnvironmentalMonitoringNetworks", 
    "EF.EnvironmentalMonitoringProgrammes", "EL.BreakLine", "EL.ContourLine", "EL.ElevationGridCoverage", "EL.ElevationTIN", 
    "EL.IsolatedArea", "EL.SpotElevation", "EL.VoidArea", "ER.FossilFuelResource", "ER.RenewableAndWastePotentialCoverage", 
    "ER.RenewableAndWasteResource", "GE._ProfileTypeValue_", "GE._StationTypeValue_", "GE._SurveyTypeValue_", 
    "GE._ThematicClassificationValue_", "GE.ActiveWell", "GE.AirborneGeophysicalSurvey", "GE.Aquiclude", "GE.Aquifer", 
    "GE.AquiferSystems", "GE.Aquitard", "GE.Borehole", "GE.BoreholeLogging", "GE.BoreholeLoggingSurvey", "GE.ConePenetrationTest", 
    "GE.CPTsurvey", "GE.FlightLine", "GE.FrequencyDomainEMSounding", "GE.FrequencyDomainEMsurvey", "GE.GeologicFault", 
    "GE.GeologicFold", "GE.GeologicUnit", "GE.GeomorphologicFeature", "GE.Geophysics.3DSeismics", "GE.GeoradarProfile", 
    "GE.GeoradarSurvey", "GE.GravityStation", "GE.GroundGravitySurvey", "GE.GroundMagneticSurvey", "GE.GroundWaterbody", 
    "GE.MagneticStation", "GE.MagnetotelluricSounding", "GE.MagnetotelluricSurvey", "GE.MultielectrodeDCProfile", "GE.RadiometricStation", 
    "GE.SeismicLine", "GE.SeismologicalStation", "GE.SeismologicalSurvey", "GE.SonarSurvey", "GE.TimeDomainEMSounding", 
    "GE.Time-domainEMsurvey", "GE.VerticalElectricSounding", "GE.VerticalSeismicProfile", "GE.VspSurvey", "GN.GeographicalNames", 
    "HB.Habitat", "HH.HealthDeterminantMeasure", "HH.HealthStatisticalData", "HY.Network", "HY.PhysicalWaters.Catchments", 
    "HY.PhysicalWaters.HydroPointOfInterest", "HY.PhysicalWaters.LandWaterBoundary", "HY.PhysicalWaters.ManMadeObject", 
    "HY.PhysicalWaters.Shore", "HY.PhysicalWaters.Waterbodies", "HY.PhysicalWaters.Wetland", "LC.LandCoverPoints", "LC.LandCoverRaster", 
    "LC.LandCoverSurfaces", "LU.ExistingLandUse", "LU.SpatialPlan", "LU.SupplementaryRegulation", "LU.ZoningElement", "MR.Mine", 
    "MR.MineralOccurrence", "NZ.ExposedElement", "OF.GridObservation", "OF.GridSeriesObservation", "OF.MultiPointObservation", 
    "OF.PointObservation", "OF.PointTimeSeriesObservation", "OI.MosaicElement", "OI.OrthoimageCoverage", "PF._EconomicActivityValue_", 
    "PF.ProductionBuilding", "PF.ProductionInstallation", "PF.ProductionInstallationPart", "PF.ProductionPlot", "PF.ProductionSite", 
    "PS.ProtectedSite", "SD._ReferenceSpeciesCodeValue_", "SO._SoilDerivedObjectParameterNameValue_", "SO.AvailableWaterCapacity", 
    "SO.AvailableWaterCapacityCoverage", "SO.BiologicalParameter", "SO.BiologicalParameterCoverage", "SO.CadmiumContent", 
    "SO.CadmiumContentCoverage", "SO.CarbonStock", "SO.CarbonStockCoverage", "SO.ChemicalParameter", "SO.ChemicalParameterCoverage", 
    "SO.ChromiumContent", "SO.ChromiumContentCoverage", "SO.CopperContent", "SO.CopperContentCoverage", "SO.LeadContent", 
    "SO.LeadContentCoverage", "SO.MercuryContent", "SO.MercuryContentCoverage", "SO.NickelContent", "SO.NickelContentCoverage", 
    "SO.NitrogenContent", "SO.NitrogenContentCoverage", "SO.ObservedSoilProfile", "SO.OrganicCarbonContent", "SO.OrganicCarbonContentCoverage", 
    "SO.PHValue", "SO.PHValueCoverage", "SO.PhysicalParameter", "SO.PhysicalParameterCoverage", "SO.PotentialRootDepth", 
    "SO.PotentialRootDepthCoverage", "SO.SoilBody", "SO.SoilSite", "SO.WaterDrainage", "SO.WaterDrainageCoverage", "SO.ZincContent", 
    "SO.ZincContentCoverage", "SR.Coastline", "SR.InterTidalArea", "SR.MarineCirculationZone", "SR.MarineContour", "SR.Sea", "SR.SeaArea", 
    "SR.SeaBedArea", "SR.SeaSurfaceArea", "SR.Shoreline", "SU.StatisticalGridCell", "SU.VectorStatisticalUnit", 
    "TN.AirTransportNetwork.AerodromeArea", "TN.AirTransportNetwork.AirLink", "TN.AirTransportNetwork.AirspaceArea", 
    "TN.AirTransportNetwork.ApronArea", "TN.AirTransportNetwork.RunwayArea", "TN.AirTransportNetwork.TaxiwayArea", 
    "TN.CableTransportNetwork.CablewayLink", "TN.CommonTransportElements.TransportArea", "TN.CommonTransportElements.TransportLink", 
    "TN.CommonTransportElements.TransportNode", "TN.RailTransportNetwork.RailwayArea", "TN.RailTransportNetwork.RailwayLink", 
    "TN.RailTransportNetwork.RailwayStationArea", "TN.RailTransportNetwork.RailwayYardArea", "TN.RoadTransportNetwork.RoadArea", 
    "TN.RoadTransportNetwork.RoadLink", "TN.RoadTransportNetwork.RoadServiceArea", "TN.RoadTransportNetwork.VehicleTrafficArea", 
    "TN.WaterTransportNetwork.FairwayArea", "TN.WaterTransportNetwork.PortArea", "TN.WaterTransportNetwork.WaterwayLink", 
    "US. OilGasChemicalsNetwork", "US._ServiceTypeValue_", "US.AdministrationForEducation", "US.AdministrationForEnvironmentalProtection", 
    "US.AdministrationForHealth", "US.AdministrationForPublicOrderAndSafety", "US.AdministrationForSocialProtection", "US.AntiFireWaterProvision", 
    "US.BachelorOrEquivalentEducation", "US.Barrack", "US.Camp", "US.CharityAndCounselling", "US.ChildCareService", "US.CivilProtectionSite", 
    "US.Defence", "US.DoctoralOrEquivalentEducation", "US.EarlyChildhoodEducation", "US.Education", "US.EducationNotElsewhereClassified", 
    "US.ElectricityNetwork", "US.EmergencyCallPoint", "US.EnvironmentalEducationCentre", "US.EnvironmentalManagementFacility", 
    "US.EnvironmentalProtection", "US.FireDetectionAndObservationSite", "US.FireProtectionService", "US.FireStation", "US.GeneralAdministrationOffice", 
    "US.GeneralHospital", "US.GeneralMedicalService", "US.Health", "US.HospitalService", "US.Housing", "US.Hydrant", "US.LowerSecondaryEducation", 
    "US.MarineRescueStation", "US.MasterOrEquivalentEducation", "US.MedicalAndDiagnosticLaboratory", "US.MedicalProductsAppliancesAndEquipment", 
    "US.NursingAndConvalescentHomeService", "US.OutpatientService", "US.ParamedicalService", "US.PoliceService", "US.PostSecondaryNonTertiaryEducation", 
    "US.PrimaryEducation", "US.PublicAdministrationOffice", "US.PublicOrderAndSafety", "US.RescueHelicopterLandingSite", "US.RescueService", 
    "US.RescueStation", "US.SewerNetwork", "US.ShortCycleTertiaryEducation", "US.Siren", "US.SocialService", "US.SpecializedAdministrationOffice", 
    "US.SpecializedHospital", "US.SpecializedMedicalServices", "US.SpecializedServiceOfSocialProtection", "US.StandaloneFirstAidEquipment", 
    "US.SubsidiaryServicesToEducation", "US.ThermalNetwork", "US.UpperSecondaryEducation", "US.UtilityNetwork", "US.WaterNetwork", "GE.VspSurevey"
};

bool is_inspire_layer_name ( std::string ln ) {
    return std::find(layer_names.begin(), layer_names.end(), ln) != layer_names.end();
}

bool is_inspire_wmts ( Layer* layer ) {

    if (! is_inspire_layer_name(layer->get_id())) {
        BOOST_LOG_TRIVIAL(debug) << "Non conforme INSPIRE WMTS (" << layer->get_id() << ") : layer name non harmonisé" ;
        return false;
    }

    if (layer->get_keywords()->size() == 0) {
        BOOST_LOG_TRIVIAL(debug) << "Non conforme INSPIRE WMTS (" << layer->get_id() << ") : pas de mots-clés" ;
        return false;
    }

    if (layer->get_metadata()->size() == 0) {
        BOOST_LOG_TRIVIAL(debug) << "Non conforme INSPIRE WMTS (" << layer->get_id() << ") : pas de métadonnées" ;
        return false;
    }

    // Pour être inspire, le style par défaut doit avoir le bon identifiant
    if (layer->get_default_style()->get_identifier() != "inspire_common:DEFAULT") {
        BOOST_LOG_TRIVIAL(debug) << "Non conforme INSPIRE WMTS (" << layer->get_id() << ") : style par défaut != inspire_common:DEFAULT" ;
        return false;
    }

    return true;
}

bool is_inspire_wms ( Layer* layer ) {

    if (! is_inspire_layer_name(layer->get_id())) {
        BOOST_LOG_TRIVIAL(debug) << "Non conforme INSPIRE WMS (" << layer->get_id() << ") : layer name non harmonisé" ;
        return false;
    }

    if (layer->get_keywords()->size() == 0) {
        BOOST_LOG_TRIVIAL(debug) << "Non conforme INSPIRE WMS (" << layer->get_id() << ") : pas de mots-clés" ;
        return false;
    }

    // Pour être inspire, le style par défaut doit avoir le bon identifiant
    if (layer->get_default_style()->get_identifier() != "inspire_common:DEFAULT") {
        BOOST_LOG_TRIVIAL(debug) << "Non conforme INSPIRE WMS (" << layer->get_id() << ") : style par défaut != inspire_common:DEFAULT" ;
        return false;
    }

    return true;
}

}
