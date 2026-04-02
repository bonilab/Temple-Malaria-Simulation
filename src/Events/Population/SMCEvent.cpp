#include "SMCEvent.h"
#include "Core/Config/Config.h"
#include "Core/Random.h"
#include "Events/ReceiveSMCTherapyEvent.h" 
#include "Model.h"
#include "Population/Population.h"
#include "Population/Properties/PersonIndexByLocationStateAgeClass.h"
#include "Population/Properties/PersonIndexAll.h"
#include "date/date.h"
#include "easylogging++.h"
#include <unordered_set>

SMCEvent::SMCEvent(const int &execute_at) {
  time = execute_at;
}

void SMCEvent::execute() {
  LOG(INFO) << date::year_month_day{scheduler->calendar_date}
            << ": executing SMCEvent";

  // const auto& tracked_ids = Model::POPULATION->smc_tracked_person_ids();
  // std::unordered_set<int> eligible_set;

  for(auto &district : districts){
    if (district < 1 || district > SpatialData::get_instance().get_boundary("district")->max_unit_id) {
      LOG(ERROR) << "District ID " << district << " is out of valid range [1, "
                 << SpatialData::get_instance().get_boundary("district")->max_unit_id << "]";
      return;
    }

    auto locations = SpatialData::get_instance().get_locations_in_unit("district", district);
    auto it = std::find(districts.begin(), districts.end(), district);
    
    // Find the corresponding index in districts
    std::size_t district_index = std::distance(districts.begin(), it);
    double fraction = fraction_population_targeted[district_index];

    auto pi_lsa = Model::POPULATION->get_person_index<PersonIndexByLocationStateAgeClass>();

    std::vector<Person*> eligible_persons;

    for (auto hs = 0; hs < Person::DEAD; hs++) {
      for (std::size_t ac = 0; ac < Model::CONFIG->number_of_age_classes(); ac++) {
        for (auto loc : locations) {
          for (auto* p : pi_lsa->vPerson()[loc][hs][ac]) {

            double age_in_years = p->age_in_floating();
            double min_age_years = age_range[0] / 12.0;
            double max_age_years = age_range[1] / 12.0;

            if (age_in_years >= min_age_years && age_in_years < max_age_years) {
                 eligible_persons.push_back(p);
            }
          
          }
        }
      }
    }

    const std::size_t total_eligible = eligible_persons.size();
    unsigned int num_targeted = Model::RANDOM->random_poisson(fraction * total_eligible);

    if (fraction >= 1.0) {
      num_targeted = total_eligible;
    } 
    else {
      num_targeted = Model::RANDOM->random_poisson(fraction * total_eligible);
      if (num_targeted > total_eligible) {
        num_targeted = total_eligible;
      }
    }


    // std::cout << "Location (District): " << district
    //           << ", Fraction targeted: " << fraction
    //           << ", Total eligible individuals: " << total_eligible
    //           << ", Number targeted for SMC final: " << num_targeted << std::endl;



    //int schedule_count = 0;
    if (!eligible_persons.empty()) {
      Model::RANDOM->shuffle(&eligible_persons[0], eligible_persons.size(), sizeof(std::size_t));
    }

    // for (auto* p : eligible_persons) {
    //   eligible_set.insert(p->get_uid());
    // }


    for (std::size_t p_i = 0; p_i < num_targeted; ++p_i) {
      auto* person = eligible_persons[p_i];

      const auto prob = Model::RANDOM->random_flat(0.0, 1.0);
      if (prob <= person->prob_present_at_smc()) {
        auto* therapy = Model::CONFIG->therapy_db()[Model::CONFIG->smc_therapy_id()];

        int days_to_receive_smc = Model::RANDOM->random_uniform(days_to_complete_all_treatments) + 1;
        int scheduled_time = Model::SCHEDULER->current_time() + days_to_receive_smc;


        ReceiveSMCTherapyEvent::schedule_event(Model::SCHEDULER, person, therapy, scheduled_time);
        //schedule_count++;
      }
    }
    // std::cout << "Number of individuals who received SMC in District " << district
    //           << ": " << schedule_count << std::endl;






  }

  // Uncomment for debugging missing tracked children

  /*

  

  // Check for missing tracked children
  std::vector<int> missing_tracked;
  std::vector<int> eligible_tracked;
  
  for (int uid : tracked_ids) {
    if (eligible_set.count(uid) == 0) {
        // tracked child is NOT eligible this round
        missing_tracked.push_back(uid);
    } else {
        eligible_tracked.push_back(uid);
    }
  }

    auto* all_person_index = Model::POPULATION->get_person_index<PersonIndexAll>();

    std::vector<Person*> tracked_missing_persons;
    std::vector<int> not_found_uids;

    for (int uid : missing_tracked) {

        bool found = false;

        // Search for the person in the index
        for (auto* person : all_person_index->vPerson()) {
            if (person->get_uid() == uid) {
                tracked_missing_persons.push_back(person);
                found = true;
                break;
            }
        }

        // If not found, record UID
        if (!found) {
            not_found_uids.push_back(uid);
        }
    }

  if (!not_found_uids.empty()) {
      std::cout << "Warning: The following tracked UIDs were not found in the population:\n";
      for (int uid : not_found_uids) {
          std::cout << "  UID " << uid << "\n";
      }
  }


  // Debug output
  std::cout << eligible_tracked.size() << " tracked children eligible "
          << "and " << missing_tracked.size() << " tracked children NOT eligible."
          << std::endl;

  if (!missing_tracked.empty()) {
    std::cout << "Detailed info for tracked children NOT eligible:\n";

    for (Person* p : tracked_missing_persons) {


        double age = p->age_in_floating();
        int loc = p->location();
        int hs  = p->host_state();

        // derive district for info
        int p_district = SpatialData::get_instance()
                             .get_admin_unit("district", loc);

        double min_age_years = age_range[0] / 12.0;
        double max_age_years = age_range[1] / 12.0;

        bool wrong_age = !(age >= min_age_years && age <= max_age_years);
        bool dead = (hs == Person::DEAD);

        std::cout << "  UID " << p->get_uid()
                  << " | Age=" << age
                  << " | Location=" << loc
                  << " | District=" << p_district
                  << " | HealthState=" << hs;

        if (wrong_age) {
            std::cout << " | REASON: age not in [" 
                      << min_age_years << ", " << max_age_years << "]";
        }

        if (dead) {
            std::cout << " | REASON: DEAD";
        }

        std::cout << "\n";
    }
}

*/


  

}