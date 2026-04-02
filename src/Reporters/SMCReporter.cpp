//#ifdef ENABLE_SMC
#include "SMCReporter.h"
#include <fmt/core.h>
#include "Model.h"
#include "Population/Properties/PersonIndexAll.h"
#include "Population/Person.h"
#include "Population/Population.h"
#include "Population/DrugsInBlood.h"
#include "Therapies/Drug.h"
#include "Core/Config/Config.h"

#include <algorithm>   
#include <random>  
#include <vector>
#include <unordered_set>
#include "Events/Population/SMCEvent.h"

SMCReporter::SMCReporter() = default;
SMCReporter::~SMCReporter() = default;

void SMCReporter::initialize(int job_number,
                                        const std::string &path) {
  output_file.open(fmt::format("{}smc_tracking_{}.txt", path, job_number));
}

void SMCReporter::before_run() {}

void SMCReporter::begin_time_step() {}

void SMCReporter::monthly_report() {
  /*
  auto* all_person_index =Model::POPULATION->get_person_index<PersonIndexAll>();

  for (auto* person : all_person_index->vPerson()) {

    //if (person->host_state() == Person::DEAD) { continue; } // previous (possibly incorrect code)
    if (person->host_state() == Person::HostStates::DEAD) { continue; }
    // if (person->age_in_floating() > 10) { continue; }
    

    //output_file << fmt::format("Date: {}, Person ID: {}, Location ID: {}, Age: {:.2f}, SP: {}\n", date::format("%Y\t%m\t%d", Model::SCHEDULER->calendar_date), person->get_uid(), person->location(), person->age_in_floating(), person->drugs_in_blood()->is_drug_in_blood(2)); 



    for (const auto &kv_drug : *person->drugs_in_blood()->drugs()) {
      //output_file << fmt::format("Date: {}, Person ID: {}, Location ID: {}, Age: {:.2f}, Drug ID: {}, Last update value: {}\n", date::format("%Y\t%m\t%d", Model::SCHEDULER->calendar_date), person->get_uid(), person->location(), person->age_in_floating(), kv_drug.first, kv_drug.second->last_update_value());
      //std::cout<<kv_drug.first<<std::endl;
      output_file << fmt::format("Date: {}, Person ID: {}, Location ID: {}, Age: {:.2f}, Drug ID: {}, present: {}\n", date::format("%Y\t%m\t%d", Model::SCHEDULER->calendar_date), person->get_uid(), person->location(), person->age_in_floating(), kv_drug.first, (person->drugs_in_blood()->is_drug_in_blood(kv_drug.first) ? "true" : "false") );

      }
    }
    */
}

int SMCReporter::get_first_smc_month() const {
    int first = 13; 

    for (const auto& day_events : Model::SCHEDULER->population_events_list_) {
        for (Event* ev : day_events) {
            auto* smc = dynamic_cast<SMCEvent*>(ev);
            if (!smc) continue;
            first = std::min(first, smc->smc_month);
        }
    }

    return first == 13 ? -1 : first;
}

void SMCReporter::custom_report(){

  // Get all person references
  auto* all_person_index = Model::POPULATION->get_person_index<PersonIndexAll>();

  //Ensure the tracked list is initialized once if refresh easch interval is set to false
  if ((Model::CONFIG->smc_refresh_samples_each_interval()) || Model::POPULATION->smc_tracked_person_ids().empty()) {
    int num_people_tracked = Model::CONFIG->smc_reporting_number_of_people_tracked();

    
    double min_age_years = 3.0/12.0;  // retrieve from config later
    double max_age_years = 60.0/12.0; // 5 years actual upper limit

    int smc_month = get_first_smc_month();

    // If no SMC months found, return without adjusting max age
    if (smc_month != -1) {

        unsigned current_month = unsigned(date::year_month_day(Model::SCHEDULER->calendar_date).month());

        int month_gap = smc_month - current_month;
        if (month_gap < 0) month_gap += 12; // When SMC starts earlier in the year than tracking date, works if we are operating inside a 12-month cycle

        // Shrink by an extra 1 month to account for cases when SMC is given near end of the month
        month_gap += 1;

        // Adjust maximum allowed age so selected kids remain <5 at SMC
        max_age_years -= (month_gap / 12.0);
    }

    // std::cout << "SMCReporter: Selecting " << num_people_tracked
    //           << " people to track, age range: "
    //           << min_age_years << " to " << max_age_years
    //           << " years." << std::endl;



    

    if (Model::CONFIG->smc_reporting_track_per_district()) {

      // number of people tracked is per district

      //collect preople by district

      std::unordered_map<int, std::vector<int>> district_to_ids;
      for (auto *person : all_person_index->vPerson()) {
        double age_in_years = person->age_in_floating();
        if (age_in_years < min_age_years || age_in_years >= max_age_years) continue;
        int district_id = SpatialData::get_instance().get_admin_unit("district", person->location());
        district_to_ids[district_id].push_back(person->get_uid());
      }

      std::random_device rd;
      std::mt19937 gen(rd());

      auto& tracked_ids = Model::POPULATION->smc_tracked_person_ids();

      // Now, for each district, shuffle and pick people
      for (const auto& kv : district_to_ids) {
        auto shuffled_ids = kv.second;
        std::shuffle(shuffled_ids.begin(), shuffled_ids.end(), gen);

        int n = std::min(num_people_tracked, static_cast<int>(shuffled_ids.size()));
        tracked_ids.insert(tracked_ids.end(), shuffled_ids.begin(), shuffled_ids.begin() + n);
      }

    }


    else{

    // number of people tracked is countrywide

    // Gather all person UIDs
    std::vector<int> all_ids;
    all_ids.reserve(all_person_index->vPerson().size());
    for (auto* person : all_person_index->vPerson()) {
        double age_in_years = person->age_in_floating();
        if (age_in_years >= min_age_years && age_in_years < max_age_years) {
                 all_ids.push_back(person->get_uid());
        }

    }

    // Shuffle randomly
    std::random_device rd;
    std::mt19937 gen(rd());
    std::shuffle(all_ids.begin(), all_ids.end(), gen);

    // Pick the first N people
    int n = std::min(num_people_tracked, static_cast<int>(all_ids.size()));

    
    auto& tracked_ids = Model::POPULATION->smc_tracked_person_ids();
    tracked_ids.assign(all_ids.begin(), all_ids.begin() + n);
    }
  }

  /*
  // comment after getting number of children for burkina faso
  int num_people_tracked = Model::CONFIG->smc_reporting_number_of_people_tracked();

    
  double min_age_years = 0; // hardcoded , retrieve from configuration
  double max_age_years = 5;

            

  // Gather all person UIDs
  std::vector<int> all_ids;
  all_ids.reserve(all_person_index->vPerson().size());
  for (auto* person : all_person_index->vPerson()) {
    double age_in_years = person->age_in_floating();
    if (age_in_years >= min_age_years && age_in_years <= max_age_years) {
          all_ids.push_back(person->get_uid());
       }
  
  }


  // Printing population under 5 at the end of each month
    auto current_date = Model::SCHEDULER->calendar_date;

    // Compute the last day of this month
    auto ymd = date::year_month_day{current_date};
    auto next_month = date::year_month{ymd.year(), ymd.month()} + date::months{1};
    auto first_day_next_month = date::sys_days{next_month / date::day{1}};
    auto last_day_this_month = first_day_next_month - date::days{1};

    // Check if current date equals the last day of the month
    if (current_date == last_day_this_month) {
        std::cout << fmt::format(
            "Total number of people with age under 5: {} in {}",
            all_ids.size(),
            date::format("%Y-%m", current_date)
        ) << std::endl;
    }
}

 // END comment after getting number of children for burkina faso
 */

  // uncomment below for actual function

  


  // Now prepare a fast lookup set (used in both cases)
  const auto& tracked_ids_vec = Model::POPULATION->smc_tracked_person_ids();
  std::unordered_set<int> tracked_ids(tracked_ids_vec.begin(), tracked_ids_vec.end());


  
  for (auto* person : all_person_index->vPerson()) {

    //if (person->host_state() == Person::DEAD) { continue; } // previous (possibly incorrect code)
    //if (person->host_state() == Person::HostStates::DEAD) { continue; }
    // if (person->age_in_floating() > 10) { continue; }
    

    //output_file << fmt::format("Date: {}, Person ID: {}, Location ID: {}, Age: {:.2f}, SP: {}\n", date::format("%Y\t%m\t%d", Model::SCHEDULER->calendar_date), person->get_uid(), person->location(), person->age_in_floating(), person->drugs_in_blood()->is_drug_in_blood(2)); 


    // all present drugs
    
    //for (const auto &kv_drug : *person->drugs_in_blood()->drugs()) {
      ////output_file << fmt::format("Date: {}, Person ID: {}, Location ID: {}, Age: {:.2f}, Drug ID: {}, Last update value: {}\n", date::format("%Y\t%m\t%d", Model::SCHEDULER->calendar_date), person->get_uid(), person->location(), person->age_in_floating(), kv_drug.first, kv_drug.second->last_update_value());
      ////std::cout<<kv_drug.first<<std::endl;
    //  output_file << fmt::format("Date: {}, Person ID: {}, Location ID: {}, Age: {:.2f}, Drug ID: {}, present: {}\n", date::format("%Y\t%m\t%d", Model::SCHEDULER->calendar_date), person->get_uid(), person->location(), person->age_in_floating(), kv_drug.first, (person->drugs_in_blood()->is_drug_in_blood(kv_drug.first) ? "true" : "false") );

    //}
    

   // only drug 2 (SP) or drug 1 (AQ)

   //uncomment below to track all people

   

    //if (person->drugs_in_blood()->is_drug_in_blood(2)) {
    //  output_file << fmt::format("Date: {}, Person ID: {}, Location ID: {}, Age: {:.2f}, Drug ID: {}, Last update value: {}\n", date::format("%Y\t%m\t%d", Model::SCHEDULER->calendar_date), person->get_uid(), person->location(), person->age_in_floating(), 2, person->drugs_in_blood()->get_drug(2)->last_update_value());
    //}

    //if (person->drugs_in_blood()->is_drug_in_blood(1)) {
    //  output_file << fmt::format("Date: {}, Person ID: {}, Location ID: {}, Age: {:.2f}, Drug ID: {}, Last update value: {}\n", date::format("%Y\t%m\t%d", Model::SCHEDULER->calendar_date), person->get_uid(), person->location(), person->age_in_floating(), 1, person->drugs_in_blood()->get_drug(1)->last_update_value());
    //}


    // end track all people

  

    // keeping track of infection and concentration of drug in bloof of people selected

    if (tracked_ids.count(person->get_uid()) > 0) {
        // Get infection state as string
        std::string state_str;
        switch (person->host_state()) {
            case Person::HostStates::SUSCEPTIBLE: state_str = "SUSCEPTIBLE"; break;
            case Person::HostStates::EXPOSED: state_str = "EXPOSED"; break;
            case Person::HostStates::ASYMPTOMATIC: state_str = "ASYMPTOMATIC"; break;
            case Person::HostStates::CLINICAL: state_str = "CLINICAL"; break;
            case Person::HostStates::DEAD: state_str = "DEAD"; break;
            default: state_str = "UNKNOWN"; break;
        }

        

        // get parasite density (reference : IndividaulsFileReporter)
        double p_density;
        if (person->all_clonal_parasite_populations()->parasites()->size() >= 1) {
        p_density = person->all_clonal_parasite_populations()
                        ->parasites()
                        ->at(0)
                        ->last_update_log10_parasite_density();
      } else {
        p_density =
            Model::CONFIG->parasite_density_level().log_parasite_density_cured;
      }

      const std::size_t parasite_population_size = person->all_clonal_parasite_populations()->size();

      // Build genotype summary for all clonal parasite populations
      std::ostringstream genotype_stream;

      for (std::size_t j = 0ul; j < parasite_population_size; j++) {
          ClonalParasitePopulation* bp =
              person->all_clonal_parasite_populations()->parasites()->at(j);

          if (j > 0) {
              genotype_stream << "; ";
          }

          genotype_stream << bp->genotype()->genotype_id()
                          << "(" << bp->last_update_log10_parasite_density() << ")";

          // std::cout << "Parasite population " << j
          //           << ": Genotype ID = " << bp->genotype()->genotype_id()
          //           << ", Log10 Parasite Density = " << bp->last_update_log10_parasite_density()
          //           << std::endl;
      }
      // std::cout<<std::endl;

      std::string genotypes = genotype_stream.str();


        // Get drug concentrations if present
        double drug0_val = 0.0, drug1_val = 0.0, drug2_val = 0.0, drug4_val = 0.0; // 0 => artemisinin, 1 => amodiaquine, 2 => SP, 4 => lumefantrine
        if (person->drugs_in_blood()->is_drug_in_blood(1)) {
            //std::cout<<"Person " << person->get_uid() << " has drug 1 in blood with value " << person->drugs_in_blood()->get_drug(1)->last_update_value() << std::endl;
            drug1_val = person->drugs_in_blood()->get_drug(1)->last_update_value();
        }
        if (person->drugs_in_blood()->is_drug_in_blood(2)) {
            //std::cout<<"Person " << person->get_uid() << " has drug 2 in blood with value " << person->drugs_in_blood()->get_drug(2)->last_update_value() << std::endl;
            drug2_val = person->drugs_in_blood()->get_drug(2)->last_update_value();
        }

        if (person->drugs_in_blood()->is_drug_in_blood(0)) {
            //std::cout<<"Person " << person->get_uid() << " has drug 0 in blood with value " << person->drugs_in_blood()->get_drug(0)->last_update_value() << std::endl;
            drug0_val = person->drugs_in_blood()->get_drug(0)->last_update_value();
        }

        if (person->drugs_in_blood()->is_drug_in_blood(4)) {
            //std::cout<<"Person " << person->get_uid() << " has drug 4 in blood with value " << person->drugs_in_blood()->get_drug(4)->last_update_value() << std::endl;
            drug4_val = person->drugs_in_blood()->get_drug(4)->last_update_value();
        }

        int district_id = SpatialData::get_instance().get_admin_unit("district", person->location());

        // Write one combined log line
        output_file << fmt::format(
            "Date: {}, ID: {}, Loc: {}, Age: {:.2f}, State: {}, Drug0: {:.4f}, Drug1: {:.4f}, Drug2: {:.4f}, Drug3: {:.4f}, ParasiteDensity: {:.4f}, Genotypes: {}\n",
            date::format("%Y-%m-%d", Model::SCHEDULER->calendar_date),
            person->get_uid(),
            district_id,
            person->age_in_floating(),
            state_str,
            drug0_val,
            drug1_val,
            drug2_val,
            drug4_val,
            p_density,
            genotypes
        );



    

    }
  }


}



void SMCReporter::after_run() {
   output_file << fmt::format("SMC Reporter After Run");

  // close the file
  if (output_file.is_open()) { output_file.close(); }
}



//#endif  // ENABLE_SMC
