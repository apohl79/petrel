#ifndef METRICS_BASIC_METRIC_H
#define METRICS_BASIC_METRIC_H

#include <cstdint>
#include <ostream>

namespace petrel {
namespace metrics {

class basic_metric {
  public:
    using pointer = std::shared_ptr<basic_metric>;

    virtual ~basic_metric() {}

    /// Aggregate counter into per second values and update the EWMA values. This function will be run in a fiber in the
    /// main thread owning the metrics registry.
    virtual void aggregate() {}

    /// The log method is called by the ostream operator<< to log a metric at a given interval.
    virtual void log(std::ostream& os) { os << "not implemented"; }

    /// The log method is called by the registry to send a metric to graphite at a given interval.
    virtual void graphite(const std::string&, std::time_t, std::ostream&) {}

    /// Hide the statistic from the log
    void hide_from_log() { m_log_visible = false; }

    /// Return true if the metric should be logged
    bool visible_in_log() const { return m_log_visible; }

  private:
    bool m_log_visible = true;
};

}  // metrics
}  // petrel

#endif  // METRICS_BASIC_METRIC_H
