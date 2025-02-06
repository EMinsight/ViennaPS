#include <geometries/psMakeHole.hpp>
#include <models/psSF6O2Etching.hpp>

#include <psProcess.hpp>
#include <psUtils.hpp>

using namespace viennaps;

int main(int argc, char *argv[]) {
  using NumericType = double;
  constexpr int D = 3;

  Logger::setLogLevel(LogLevel::INTERMEDIATE);
  omp_set_num_threads(16);

  // Parse the parameters
  utils::Parameters params;
  if (argc > 1) {
    params.readConfigFile(argv[1]);
  } else {
    std::cout << "Usage: " << argv[0] << " <config file>" << std::endl;
    return 1;
  }

  // set parameter units
  units::Length::setUnit(params.get<std::string>("lengthUnit"));
  units::Time::setUnit(params.get<std::string>("timeUnit"));

  // geometry setup
  auto geometry = SmartPointer<Domain<NumericType, D>>::New();
  MakeHole<NumericType, D>(
      geometry, params.get("gridDelta"), params.get("xExtent"),
      params.get("yExtent"), params.get("holeRadius"), params.get("maskHeight"),
      params.get("taperAngle"), 0 /* base height */,
      false /* periodic boundary */, true /*create mask*/, Material::Si)
      .apply();

  // use pre-defined model SF6O2 etching model
  SF6O2Parameters<NumericType> modelParams;
  modelParams.ionFlux = params.get("ionFlux");
  modelParams.etchantFlux = params.get("etchantFlux");
  modelParams.oxygenFlux = params.get("oxygenFlux");
  modelParams.Ions.meanEnergy = params.get("meanEnergy");
  modelParams.Ions.sigmaEnergy = params.get("sigmaEnergy");
  modelParams.Ions.exponent = params.get("ionExponent");
  modelParams.Passivation.A_ie = params.get("A_O");
  modelParams.Si.A_ie = params.get("A_Si");
  modelParams.etchStopDepth = params.get("etchStopDepth");
  auto model = SmartPointer<SF6O2Etching<NumericType, D>>::New(modelParams);

  // process setup
  Process<NumericType, D> process;
  process.setDomain(geometry);
  process.setProcessModel(model);
  process.setMaxCoverageInitIterations(20);
  process.setNumberOfRaysPerPoint(params.get("raysPerPoint"));
  process.setProcessDuration(params.get("processTime"));
  process.setIntegrationScheme(
      params.get<viennals::IntegrationSchemeEnum>("integrationScheme"));

  // print initial surface
  geometry->saveSurfaceMesh("initial.vtp");

  // run the process
  process.apply();

  // print final surface
  geometry->saveSurfaceMesh(params.get<std::string>("outputFile"));
}
